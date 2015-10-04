#pragma once
#include "afxwin.h"


// RecorderDlg 대화 상자입니다.

class RecorderDlg : public CDialogEx
{
	DECLARE_DYNAMIC(RecorderDlg)

public:
	RecorderDlg(CString siteUuid, CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~RecorderDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_RECORDERDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
private:
	CString m_strSiteUuid;
	CEdit m_editRecorderUuid;
	CEdit m_editRecorderName;
	CEdit m_editRecorderAddress;
	CEdit m_editRecorderUsername;
	CEdit m_editRecorderPassword;
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedButtonGenerateUuid();
};
