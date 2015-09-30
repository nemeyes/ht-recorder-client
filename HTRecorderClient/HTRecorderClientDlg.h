
// HTRecorderClientDlg.h : header file
//

#pragma once
#include "afxcmn.h"
#include "afxbutton.h"
#include "afxwin.h"

#include "GdipButton.h"


// CHTRecorderClientDlg dialog
class CHTRecorderClientDlg : public CDialogEx
{
// Construction
public:
	CHTRecorderClientDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_HTRECORDERCLIENT_DIALOG };

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
private:

	CTreeCtrl _sites;
	CGdipButton _btn_setup;
public:
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};
