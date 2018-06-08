#pragma once


// ZoomWindow-Dialogfeld

class ZoomWindow : public CDialogEx
{
	DECLARE_DYNAMIC(ZoomWindow)

public:
	ZoomWindow(CWnd* pParent = NULL);   // Standardkonstruktor
	virtual ~ZoomWindow();

// Dialogfelddaten
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV-Unterstützung

	DECLARE_MESSAGE_MAP()
};
