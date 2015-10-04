#pragma once
#include "afxwin.h"


// SiteDlg 대화 상자입니다.

class SiteDlg : public CDialogEx
{
	DECLARE_DYNAMIC(SiteDlg)

public:
	SiteDlg(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~SiteDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_SITEDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
private:
	CEdit m_editSiteUuid;
	CEdit m_editSiteName;
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedButtonGenerateSiteUuid();
private:
	
};
