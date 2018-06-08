
// MagCaptureDlg.h : header file
//

#pragma once

#include <opencv2\core\core.hpp>
#include "Magnification.h"
#include "HostDlg.h"

BOOL CALLBACK MagImageScaling(HWND hwnd, void *srcdata, MAGIMAGEHEADER srcheader,
					 void *destdata, MAGIMAGEHEADER destheader, 
					 RECT unclipped, RECT clipped, HRGN dirty);

// CMagCaptureDlg dialog
class CMagCaptureDlg : public CDialogEx
{
// Construction
public:
	CMagCaptureDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_MAGCAPTURE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
    static HWND hwndMag;
	afx_msg void OnBnClickedCaptureButton();
	afx_msg void OnDestroy();

protected:
	CHostDlg* hostDlg;
	HINSTANCE hInstance;
	HWND *pFilterList;
};
