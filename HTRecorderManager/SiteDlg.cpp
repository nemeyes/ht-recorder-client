// SiteDlg.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "HTRecorderManager.h"
#include "SiteDlg.h"
#include "afxdialogex.h"
#include "GUIDGenerator.h"
#include "SiteDAO.h"
#include "StringUtil.h"


// SiteDlg 대화 상자입니다.

IMPLEMENT_DYNAMIC(SiteDlg, CDialogEx)

SiteDlg::SiteDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(SiteDlg::IDD, pParent)
{

}

SiteDlg::~SiteDlg()
{
}

void SiteDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_SITE_UUID, m_editSiteUuid);
	DDX_Control(pDX, IDC_EDIT_SITE_NAME, m_editSiteName);
}


BEGIN_MESSAGE_MAP(SiteDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &SiteDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &SiteDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_GENERATE_SITE_UUID, &SiteDlg::OnBnClickedButtonGenerateSiteUuid)
END_MESSAGE_MAP()


// SiteDlg 메시지 처리기입니다.


void SiteDlg::OnBnClickedOk()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString site_uuid;
	CString site_name;
	m_editSiteUuid.GetWindowTextW(site_uuid);
	m_editSiteName.GetWindowTextW(site_name);

	if (site_uuid.GetLength() < 1 || site_name.GetLength() < 1)
		return;

	SITE_T site;
	wcscpy(site.uuid, (LPWSTR)(LPCWSTR)site_uuid);
	wcscpy(site.name, (LPWSTR)(LPCWSTR)site_name);

	SiteDAO dao;
	dao.CreateSite(&site);

	CDialogEx::OnOK();
}


void SiteDlg::OnBnClickedCancel()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CDialogEx::OnCancel();
}


void SiteDlg::OnBnClickedButtonGenerateSiteUuid()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_editSiteUuid.SetWindowText(GuidGenerator::generate_widechar_string_guid());
}
