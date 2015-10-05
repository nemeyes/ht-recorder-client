
#include "stdafx.h"
#include "mainfrm.h"
#include "SiteView.h"
#include "Resource.h"
#include "HTRecorderManager.h"
#include "SiteDAO.h"
#include "RecorderDAO.h"
#include "SiteDlg.h"
#include "RecorderDlg.h"

#include "HTRecorderFactory.h"
#include "HTRecorderManagerDoc.h"
#include "HTRecorderManagerView.h"

#define TREE_LEVEL_ROOT 0
#define TREE_LEVEL_SITE 1
#define TREE_LEVEL_RECORDER 2
#define TREE_LEVEL_CAMERA 3

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CSiteView

CSiteView::CSiteView()
	: m_sites(0)
	, m_sitesCount(0)
{
}

CSiteView::~CSiteView()
{
	RemoveAllSites();
}

BEGIN_MESSAGE_MAP(CSiteView, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_PROPERTIES, OnProperties)
	ON_COMMAND(ID_ADD_SITE, OnAddSite)
	ON_COMMAND(ID_REMOVE_SITE, OnRemoveSite)
	ON_COMMAND(ID_ADD_RECORDER, OnAddRecorder)
	ON_COMMAND(ID_REMOVE_RECORDER, OnRemoveRecorder)

/*	ON_COMMAND(ID_ADD_CAMERA, OnAddCamera)
	//ON_COMMAND(ID_REMOVE_CAMERA, OnRemoveCamera)
	ON_COMMAND(ID_START_RECORDING, OnStartRecording)
	ON_COMMAND(ID_STOP_RECORDING, OnStopRecording)*/

	ON_COMMAND(ID_PLAY_RELAY, OnPlayRelay)
	ON_COMMAND(ID_PLAY_PLAYBACK, OnPlayPlayback)
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWorkspaceBar message handlers

int CSiteView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// Create view:
	const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS;

	if (!m_wndSiteView.Create(dwViewStyle, rectDummy, this, 4))
	{
		TRACE0("Failed to create file view\n");
		return -1;      // fail to create
	}

	// Load view images:
	m_FileViewImages.Create(IDB_SITE_VIEW, 16, 0, RGB(255, 255, 255));
	m_wndSiteView.SetImageList(&m_FileViewImages, TVSIL_NORMAL);

	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_EXPLORER);
	m_wndToolBar.LoadToolBar(IDR_EXPLORER, 0, 0, TRUE /* Is locked */);

	OnChangeVisualStyle();

	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);

	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));

	m_wndToolBar.SetOwner(this);

	// All commands will be routed via this control , not via the parent frame:
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

	// Fill in some static tree view data (dummy code, nothing magic here)
	FillSiteView();
	AdjustLayout();

	return 0;
}

void CSiteView::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CSiteView::RemoveAllSites()
{
	for (int index = 0; index < m_sitesCount; index++)
	{
		for (int recorderIndex = 0; recorderIndex < m_sites[index]->recordersCount; recorderIndex++)
		{
			for (int camIndex = 0; camIndex < m_sites[index]->recorders[recorderIndex]->camerasCount; camIndex++)
			{
				free(m_sites[index]->recorders[recorderIndex]->cameras[camIndex]);
				m_sites[index]->recorders[recorderIndex]->cameras[camIndex] = 0;
			}
			free(m_sites[index]->recorders[recorderIndex]->cameras);
			m_sites[index]->recorders[recorderIndex]->cameras = 0;
			m_sites[index]->recorders[recorderIndex]->camerasCount = 0;

			free(m_sites[index]->recorders[recorderIndex]);
			m_sites[index]->recorders[recorderIndex] = 0;
		}
		free(m_sites[index]->recorders);
		m_sites[index]->recorders = 0;
		m_sites[index]->recordersCount = 0;

		free(m_sites[index]);
		m_sites[index] = 0;
	}
	free(m_sites);
	m_sites = 0;
	m_sitesCount = 0;
}

void CSiteView::FillSiteView()
{
	m_wndSiteView.DeleteAllItems();
	RemoveAllSites();


	HTREEITEM hTreeRoot = m_wndSiteView.InsertItem(_T("부산교통공사1호선"), 0, 0);
	m_wndSiteView.SetItemState(hTreeRoot, TVIS_BOLD, TVIS_BOLD);

	SiteDAO siteDao;
	siteDao.RetrieveSites(&m_sites, m_sitesCount);
	for (int index = 0; index < m_sitesCount; index++)
	{
		HTREEITEM hTreeSite = m_wndSiteView.InsertItem(m_sites[index]->name, 1, 1, hTreeRoot);
		m_sites[index]->tree_level = TREE_LEVEL_SITE;
		m_wndSiteView.SetItemData(hTreeSite, (DWORD_PTR)m_sites[index]);

		m_sites[index]->recorders = 0;
		m_sites[index]->recordersCount = 0;
		RecorderDAO recorderDao;
		recorderDao.RetrieveRecorder(m_sites[index], &(m_sites[index]->recorders), m_sites[index]->recordersCount);

		for (int recorderIndex = 0; recorderIndex < m_sites[index]->recordersCount; recorderIndex++)
		{
			HTREEITEM hTreeRecorder = m_wndSiteView.InsertItem(m_sites[index]->recorders[recorderIndex]->name, 2, 2, hTreeSite);
			m_sites[index]->recorders[recorderIndex]->parent = m_sites[index];
			m_sites[index]->recorders[recorderIndex]->tree_level = TREE_LEVEL_RECORDER;
			m_sites[index]->recorders[recorderIndex]->cameras = 0;
			m_sites[index]->recorders[recorderIndex]->camerasCount = 0;
			m_wndSiteView.SetItemData(hTreeRecorder, (DWORD_PTR)m_sites[index]->recorders[recorderIndex]);


			HTRecorder * recorder = HTRecorderFactory::GetInstance().GetRecorder(m_sites[index]->recorders[recorderIndex]->uuid, m_sites[index]->recorders[recorderIndex]->address, m_sites[index]->recorders[recorderIndex]->username, m_sites[index]->recorders[recorderIndex]->pwd);
			if (recorder)
			{
				RS_DEVICE_INFO_SET_T rsDeviceInfos;
				if (recorder->GetDeviceList(&rsDeviceInfos))
				{
					m_sites[index]->recorders[recorderIndex]->camerasCount = rsDeviceInfos.validDeviceCount;
					m_sites[index]->recorders[recorderIndex]->cameras = static_cast<CAMERA_T**>(malloc(rsDeviceInfos.validDeviceCount*sizeof(CAMERA_T*)));
					for (int camIndex = 0; camIndex < rsDeviceInfos.validDeviceCount; camIndex++)
					{
						CString strUuid = rsDeviceInfos.deviceInfo[camIndex].GetID();
						//UINT keyid = rsDeviceInfos.deviceInfo[camIndex].GetKeyID();
						CString strAddress = rsDeviceInfos.deviceInfo[camIndex].GetURL();
						CString strUsername = rsDeviceInfos.deviceInfo[camIndex].GetUser();
						CString strPassword = rsDeviceInfos.deviceInfo[camIndex].GetPassword();
						m_sites[index]->recorders[recorderIndex]->cameras[camIndex] = static_cast<CAMERA_T*>(malloc(sizeof(CAMERA_T)));
						wcscpy(m_sites[index]->recorders[recorderIndex]->cameras[camIndex]->uuid, strUuid);
						memset(m_sites[index]->recorders[recorderIndex]->cameras[camIndex]->key, 0x00, sizeof(m_sites[index]->recorders[recorderIndex]->cameras[camIndex]->key));
						m_sites[index]->recorders[recorderIndex]->cameras[camIndex]->key[7] = (BYTE)rsDeviceInfos.deviceInfo[camIndex].GetKeyID();

						wcscpy(m_sites[index]->recorders[recorderIndex]->cameras[camIndex]->address, strAddress);
						wcscpy(m_sites[index]->recorders[recorderIndex]->cameras[camIndex]->username, strUsername);
						wcscpy(m_sites[index]->recorders[recorderIndex]->cameras[camIndex]->pwd, strPassword);

						HTREEITEM hTreeCamera = m_wndSiteView.InsertItem(m_sites[index]->recorders[recorderIndex]->cameras[camIndex]->address, 3, 3, hTreeRecorder);

						m_sites[index]->recorders[recorderIndex]->cameras[camIndex]->parent = m_sites[index]->recorders[recorderIndex];
						m_sites[index]->recorders[recorderIndex]->cameras[camIndex]->tree_level = TREE_LEVEL_CAMERA;
						m_wndSiteView.SetItemData(hTreeCamera, (DWORD_PTR)m_sites[index]->recorders[recorderIndex]->cameras[camIndex]);
					}
				}
			}
		}
	}


	/*HTREEITEM hSite = m_wndSiteView.InsertItem(_T("신평"), 1, 1, hRoot);
	m_wndSiteView.SetItemData(hSite, 1);
	
	HTREEITEM hRecorder = m_wndSiteView.InsertItem(_T("녹화서버1"), 2, 2, hSite);
	m_wndSiteView.SetItemData(hRecorder, 2);
	hRecorder = m_wndSiteView.InsertItem(_T("녹화서버2"), 2, 2, hSite);
	m_wndSiteView.SetItemData(hRecorder, 2);
	hRecorder = m_wndSiteView.InsertItem(_T("녹화서버3"), 2, 2, hSite);
	m_wndSiteView.SetItemData(hRecorder, 2);
	hRecorder = m_wndSiteView.InsertItem(_T("녹화서버4"), 2, 2, hSite);
	m_wndSiteView.SetItemData(hRecorder, 2);

	hSite = m_wndSiteView.InsertItem(_T("당리"), 1, 1, hRoot);

	hRecorder = m_wndSiteView.InsertItem(_T("녹화서버5"), 2, 2, hSite);
	m_wndSiteView.SetItemData(hRecorder, 2);
	hRecorder = m_wndSiteView.InsertItem(_T("녹화서버6"), 2, 2, hSite);
	m_wndSiteView.SetItemData(hRecorder, 2);
	hRecorder = m_wndSiteView.InsertItem(_T("녹화서버7"), 2, 2, hSite);
	m_wndSiteView.SetItemData(hRecorder, 2);



	/*HTREEITEM hRes = m_wndSiteView.InsertItem(_T("FakeApp Resource Files"), 0, 0, hRoot);

	m_wndSiteView.InsertItem(_T("FakeApp.ico"), 2, 2, hRes);
	m_wndSiteView.InsertItem(_T("FakeApp.rc2"), 2, 2, hRes);
	m_wndSiteView.InsertItem(_T("FakeAppDoc.ico"), 2, 2, hRes);
	m_wndSiteView.InsertItem(_T("FakeToolbar.bmp"), 2, 2, hRes);*/

	m_wndSiteView.Expand(hTreeRoot, TVE_EXPAND);
	//m_wndSiteView.Expand(hSrc, TVE_EXPAND);
	//m_wndSiteView.Expand(hInc, TVE_EXPAND);
}

void CSiteView::OnContextMenu(CWnd* pWnd, CPoint point)
{
	CTreeCtrl* pWndTree = (CTreeCtrl*)&m_wndSiteView;
	ASSERT_VALID(pWndTree);

	if (pWnd != pWndTree)
	{
		CDockablePane::OnContextMenu(pWnd, point);
		return;
	}

	if (point != CPoint(-1, -1))
	{
		// Select clicked item:
		CPoint ptTree = point;
		pWndTree->ScreenToClient(&ptTree);

		UINT flags = 0;
		HTREEITEM hTreeItem = pWndTree->HitTest(ptTree, &flags);
		if (hTreeItem != NULL)
		{
			pWndTree->SelectItem(hTreeItem);
		}
	}
	HTREEITEM item = pWndTree->GetSelectedItem();
	TREE_T * item_data = (TREE_T*)pWndTree->GetItemData(item);
	pWndTree->SetFocus();

	if (item_data == NULL)
		theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_ROOT, point.x, point.y, this, TRUE);
	else if (item_data->tree_level == TREE_LEVEL_SITE)
		theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_SITES, point.x, point.y, this, TRUE);
	else if (item_data->tree_level == TREE_LEVEL_RECORDER)
		theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_RECORDER, point.x, point.y, this, TRUE);
	else if (item_data->tree_level==TREE_LEVEL_CAMERA)
		theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_CAMERA, point.x, point.y, this, TRUE);

}

void CSiteView::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

	m_wndToolBar.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndSiteView.SetWindowPos(NULL, rectClient.left + 1, rectClient.top + cyTlb + 1, rectClient.Width() - 2, rectClient.Height() - cyTlb - 2, SWP_NOACTIVATE | SWP_NOZORDER);
}

void CSiteView::OnProperties()
{
	AfxMessageBox(_T("Properties...."));

}

void CSiteView::OnAddSite()
{
	SiteDlg dlg;
	dlg.DoModal();
	FillSiteView();
}

void CSiteView::OnRemoveSite()
{
	CTreeCtrl* pWndTree = (CTreeCtrl*)&m_wndSiteView;
	HTREEITEM hTreeItem = pWndTree->GetSelectedItem();
	SITE_T * site = (SITE_T*)pWndTree->GetItemData(hTreeItem);

	SiteDAO siteDao;
	siteDao.DeleteSite(site);

	FillSiteView();
}

void CSiteView::OnAddRecorder()
{
	CTreeCtrl* pWndTree = (CTreeCtrl*)&m_wndSiteView;
	HTREEITEM hTreeItem = pWndTree->GetSelectedItem();
	SITE_T * site = (SITE_T*)pWndTree->GetItemData(hTreeItem);

	RecorderDlg dlg(site->uuid);
	dlg.DoModal();
	FillSiteView();
}

void CSiteView::OnRemoveRecorder()
{
	CTreeCtrl* pWndTree = (CTreeCtrl*)&m_wndSiteView;
	HTREEITEM hTreeItem = pWndTree->GetSelectedItem();
	RECORDER_T * recorder = (RECORDER_T*)pWndTree->GetItemData(hTreeItem);

	RecorderDAO recorderDao;
	recorderDao.DeleteRecorder(recorder->parent, recorder);

	FillSiteView();
}

void CSiteView::OnAddCamera()
{

}

void CSiteView::OnRemoveCamera()
{

}

void CSiteView::OnStartRecording()
{

}

void CSiteView::OnStopRecording()
{

}

void CSiteView::OnPlayRelay()
{
	CFrameWnd * pFrame = (CFrameWnd *)(AfxGetApp()->m_pMainWnd);
	CHTRecorderManagerView * videoView = (CHTRecorderManagerView*)pFrame->GetActiveView();
	if (videoView)
	{
		CTreeCtrl* pWndTree = (CTreeCtrl*)&m_wndSiteView;
		HTREEITEM hTreeItem = pWndTree->GetSelectedItem();
		CAMERA_T * camera = (CAMERA_T*)pWndTree->GetItemData(hTreeItem);
		
		videoView->StartRelay(camera->parent->uuid, camera->parent->address, camera->parent->username, camera->parent->pwd, camera->uuid, camera->key, 0);
	}
}

void CSiteView::OnPlayPlayback()
{

}

void CSiteView::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect rectTree;
	m_wndSiteView.GetWindowRect(rectTree);
	ScreenToClient(rectTree);

	rectTree.InflateRect(1, 1);
	dc.Draw3dRect(rectTree, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DSHADOW));
}

void CSiteView::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);

	m_wndSiteView.SetFocus();
}

void CSiteView::OnChangeVisualStyle()
{
	m_wndToolBar.CleanUpLockedImages();
	m_wndToolBar.LoadBitmap(theApp.m_bHiColorIcons ? IDB_EXPLORER_24 : IDR_EXPLORER, 0, 0, TRUE /* Locked */);

	m_FileViewImages.DeleteImageList();

	UINT uiBmpId = theApp.m_bHiColorIcons ? IDB_FILE_VIEW_24 : IDB_FILE_VIEW;

	CBitmap bmp;
	if (!bmp.LoadBitmap(uiBmpId))
	{
		TRACE(_T("Can't load bitmap: %x\n"), uiBmpId);
		ASSERT(FALSE);
		return;
	}

	BITMAP bmpObj;
	bmp.GetBitmap(&bmpObj);

	UINT nFlags = ILC_MASK;

	nFlags |= (theApp.m_bHiColorIcons) ? ILC_COLOR24 : ILC_COLOR4;

	m_FileViewImages.Create(16, bmpObj.bmHeight, nFlags, 0, 0);
	m_FileViewImages.Add(&bmp, RGB(255, 255, 255));

	m_wndSiteView.SetImageList(&m_FileViewImages, TVSIL_NORMAL);
}


