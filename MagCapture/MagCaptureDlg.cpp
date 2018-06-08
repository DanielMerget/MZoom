
// MagCaptureDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MagCapture.h"
#include "MagCaptureDlg.h"
#include "afxdialogex.h"
#include "ZoomWindow.h"
#include "ThreadPool.h"
#include <opencv2\imgproc\imgproc.hpp>
#include <thread>
#include <mutex>
#include <future>

HWND CMagCaptureDlg::hwndMag = NULL;
static int g_ScreenX = 1920; // default only
static int g_ScreenY = 1080; // default only
static float g_factor = 1.00f; // default only
static int g_interpolate = cv::INTER_LINEAR;

static cv::Rect ROI;
static RECT roi;
static LONG offsetx = 0;
static LONG offsety = 0;
static RECT screenRect;
static BITMAP Screen;

// Frame buffers
static cv::Mat frame[2];
static cv::Mat* showFrame = &frame[0];
static cv::Mat* waitingFrame = &frame[1]; // currently unused, since it did not help improve performance

static const int render_threads = 4;
static const int buffers = 2 * render_threads;
static cv::Mat renderbuf[buffers];
static cv::Mat* renderbuf_ptr[buffers];

// Threading related globalds
static std::condition_variable listener;
static std::mutex mut;
static std::mutex rendermut;
static bool new_frame = false;
static bool stopLoopNow = false;
static const ZoomWindow* linkToZoomWindow = NULL;

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();
   	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

CMagCaptureDlg::CMagCaptureDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMagCaptureDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMagCaptureDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMagCaptureDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_CAPTURE_BUTTON, &CMagCaptureDlg::OnBnClickedCaptureButton)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CMagCaptureDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	hostDlg = NULL;
	pFilterList = NULL;

	RECT rect;
	HWND hDesktop = ::GetDesktopWindow();
	::GetWindowRect(hDesktop, &rect);
    g_ScreenX = rect.right;
    g_ScreenY = rect.bottom;

	hInstance = AfxGetInstanceHandle();

	if (!MagInitialize())
	{
		return FALSE;
	}

	hostDlg = new CHostDlg();
	hostDlg->Create();
	hwndMag = CreateWindow(WC_MAGNIFIER, TEXT("MagnifierWindow"), 
		WS_CHILD | WS_VISIBLE,
        0, 0, g_ScreenX, g_ScreenY,
		hostDlg->GetSafeHwnd(), NULL, hInstance, NULL );
	if (hwndMag == NULL)
	{
		return FALSE;
	}

	// Set the callback function
    // MagImageScaling(...) will be called whenever MagSetWindowSource() is invoked.
    // The callback is ASYNCHRONOUS, which means that we cannot simply wait for it using a mutex,
    // which drastically limits the maximum framerate we can achieve. :-(
    // Unfortunately, since this is (deprecated) windows API, we cannot change it, nor expect it to be changed
	if (!MagSetImageScalingCallback(hwndMag, (MagImageScalingCallback)MagImageScaling))
	{
		return FALSE;
	}

    // Get the screen rectangle
    screenRect.right = g_ScreenX;
    screenRect.bottom = g_ScreenY;
    screenRect.top = 0;
    screenRect.left = 0;

    Screen.bmType = 0;
    Screen.bmWidth = g_ScreenX;
    Screen.bmHeight = g_ScreenY;
    Screen.bmWidthBytes = g_ScreenX * 4;
    Screen.bmPlanes = 1;
    Screen.bmBitsPixel = 32;

    for (int i = 0; i < buffers; ++i) {
        renderbuf_ptr[i] = &renderbuf[i];
    }

	return TRUE;
}

void CMagCaptureDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

void CMagCaptureDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CMagCaptureDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMagCaptureDlg::OnDestroy()
{
    stopLoopNow = true; // should have happened already, but just to make sure
    CDialogEx::OnDestroy();

    // Free memory
    if (hostDlg != NULL) {
        hostDlg->DestroyWindow();
    }
    if (hwndMag != NULL) {
        ::DestroyWindow(hwndMag);
    }
    delete[] pFilterList;
    hwndMag = NULL;
    delete hostDlg;

    MagUninitialize();
}

void adjustROI() {
    roi.right = g_ScreenX / 2 + (int)((float)(g_ScreenX / 2) / g_factor);
    roi.bottom = g_ScreenY / 2 + (int)((float)(g_ScreenY / 2) / g_factor);
    roi.top = g_ScreenY - roi.bottom;
    roi.left = g_ScreenX - roi.right;
    offsetx = std::max(offsetx, -roi.left);
    offsetx = std::min(offsetx, roi.left);
    offsety = std::min(offsety, roi.top);
    offsety = std::max(offsety, -roi.top);
    ROI = cv::Rect(roi.left + offsetx, roi.top + offsety, roi.right - roi.left, roi.bottom - roi.top);
}

void processInput(ZoomWindow* zw) {
    MSG mssg;
    if (PeekMessage(&mssg, zw->GetSafeHwnd(), 0, 0, PM_REMOVE))
    {
        if (mssg.message == WM_KEYDOWN)
        {
            char c = MapVirtualKey((UINT)mssg.wParam, MAPVK_VK_TO_CHAR);
            switch (c) {
            case '+':
                g_factor *= 1.0f + 2.0f / g_ScreenY;
                break;
            case '-':
                g_factor /= 1.0f + 2.0f / g_ScreenY;
                if (g_factor < 1.0f) g_factor = 1.0f;
                break;
            case '*':
                g_factor *= pow(1.0f + 2.0f / g_ScreenY, 5);
                break;
            case '/':
                g_factor /= pow(1.0f + 2.0f / g_ScreenY, 5);
                if (g_factor < 1.0f) g_factor = 1.0f;
                break;
            case 'A':
                offsetx -= 1;
                break;
            case 'D':
                offsetx += 1;
                break;
            case 'W':
                offsety -= 1;
                break;
            case 'S':
                offsety += 1;
                break;
            case '4':
                offsetx -= (LONG)(8 * g_factor);
                break;
            case '6':
                offsetx += (LONG)(8 * g_factor);
                break;
            case '8':
                offsety -= (LONG)(8 * g_factor);
                break;
            case '5':
                offsety += (LONG)(8 * g_factor);
                break;
            case 'R':
                // reset
                g_factor = 1.0f;
                break;
            case 'C':
                // center
                offsetx = 0;
                offsety = 0;
                break;
            case 'I':
                // switch between cv::INTER_LINEAR and CV::INTER_NEAREST
                g_interpolate = 1 - g_interpolate;
                break;
            default:
                stopLoopNow = true;
                break;
            }
            adjustROI();
        }
    }
}

void renderLoop(HWND hwnd, const RECT& rect) {
    // to avoid infinite loops during debugging, use the following
    // (highly recommended unless you like frequently restarting your OS)
    //int i = 0;
    //while (!stopLoopNow && ++i <= 1000) {
    while (!stopLoopNow) {
        MagSetWindowSource(hwnd, rect); // we invode the callback function, MagImageScaling(...)
    }
    stopLoopNow = true;
}

void viewLoop(HWND hwnd, const RECT& rect) {
    ZoomWindow* zw = new ZoomWindow;
    zw->Create(IDD_DIALOG1, NULL);
    zw->ShowWindow(SW_SHOWMAXIMIZED);
    linkToZoomWindow = zw;
    while (!stopLoopNow) {
        processInput(zw);

        {
            std::unique_lock<std::mutex> lock(mut);
            listener.wait(lock, []{return new_frame; }); // wait until a new frame was rendered
            Screen.bmBits = (void*)showFrame->data; // and set the pointer to the rendered data
            new_frame = false;
        }
        
        HBITMAP hbm = CreateBitmapIndirect(&Screen);
        zw->SetBackgroundImage(hbm, CDialogEx::BackgroundLocation::BACKGR_TILE);
    }
    stopLoopNow = true;
    delete zw;
    linkToZoomWindow = NULL;
    zw = NULL;
}

void CMagCaptureDlg::OnBnClickedCaptureButton()
{
    MagSetWindowSource(hwndMag, screenRect);
    adjustROI();

    std::thread viewThread(viewLoop, hwndMag, screenRect);

    while (linkToZoomWindow == NULL) {
        Sleep(1); // Wait for viewThread
    }

    // Setup the filter list to exclude the main window
    delete [] pFilterList;
    pFilterList = new HWND[2];
    pFilterList[0] = this->GetSafeHwnd();
    pFilterList[1] = linkToZoomWindow->GetSafeHwnd();

    // Filter the main window (so that it is basically invisible)
    if (!MagSetWindowFilterList(hwndMag, MW_FILTERMODE_EXCLUDE, 2, pFilterList)) return;

    renderLoop(hwndMag, screenRect);
    viewThread.join();
    stopLoopNow = false;
}

void scaleImage(void *img) {
    static std::atomic<int> i = 0;
    static const auto sz = cv::Mat(g_ScreenY, g_ScreenX, CV_8UC4, img, 0).size();

    const auto j = ++i % buffers;
    cv::resize(cv::Mat(g_ScreenY, g_ScreenX, CV_8UC4, img, 0)(ROI), *renderbuf_ptr[j], sz, /* offx */ 0.0, /* offy */ 0.0, g_interpolate);

    {
        std::unique_lock<std::mutex> lock(mut);
        //std::swap(waitingFrame, renderbuf_ptr[j]);
        std::swap(showFrame, renderbuf_ptr[j]);
        new_frame = true;
    }

    listener.notify_one();
}

BOOL CALLBACK MagImageScaling(HWND hwnd, void *srcdata, MAGIMAGEHEADER srcheader, void *destdata, MAGIMAGEHEADER destheader, RECT unclipped, RECT clipped, HRGN dirty) {
    static ThreadPool pool(render_threads);
    pool.enqueue(scaleImage, srcdata);
	return TRUE;
}
