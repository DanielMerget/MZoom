// ZoomWindow.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "MagCapture.h"
#include "ZoomWindow.h"
#include "afxdialogex.h"


// ZoomWindow-Dialogfeld

IMPLEMENT_DYNAMIC(ZoomWindow, CDialogEx)

ZoomWindow::ZoomWindow(CWnd* pParent /*=NULL*/)
	: CDialogEx(ZoomWindow::IDD, pParent)
{

}

ZoomWindow::~ZoomWindow()
{
}

void ZoomWindow::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(ZoomWindow, CDialogEx)
END_MESSAGE_MAP()


// ZoomWindow-Meldungshandler
