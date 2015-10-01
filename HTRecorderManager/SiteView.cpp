
#include "stdafx.h"
#include "mainfrm.h"
#include "SiteView.h"
#include "Resource.h"
#include "HTRecorderManager.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CSiteView

CSiteView::CSiteView()
{
}

CSiteView::~CSiteView()
{
}

BEGIN_MESSAGE_MAP(CSiteView, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_PROPERTIES, OnProperties)
	ON_COMMAND(ID_ADD_SITE, OnAddSite)
	ON_COMMAND(ID_REMOVE_SITE, OnRemoveSite)

	ON_COMMAND(ID_OPEN, OnFileOpen)
	ON_COMMAND(ID_OPEN_WITH, OnFileOpenWith)
	ON_COMMAND(ID_DUMMY_COMPILE, OnDummyCompile)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CLEAR, OnEditClear)
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
	m_FileViewImages.Create(IDB_SITE_VIEW, 24, 0, RGB(255, 255, 255));
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

void CSiteView::FillSiteView()
{
	HTREEITEM hRoot = m_wndSiteView.InsertItem(_T("부산교통공사1호선"), 0, 0);
	m_wndSiteView.SetItemState(hRoot, TVIS_BOLD, TVIS_BOLD);
	m_wndSiteView.SetItemData(hRoot, 0);

	HTREEITEM hSite = m_wndSiteView.InsertItem(_T("신평"), 1, 1, hRoot);
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

	m_wndSiteView.Expand(hRoot, TVE_EXPAND);
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
	DWORD item_data = pWndTree->GetItemData(item);
	pWndTree->SetFocus();

	if (item_data == 1)
	{
		theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_ROOT, point.x, point.y, this, TRUE);
	}

	//theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EXPLORER, point.x, point.y, this, TRUE);
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
	// TODO: Add your command handler code here
	int a = 2;
}

void CSiteView::OnRemoveSite()
{
	// TODO: Add your command handler code here
	int a = 2;
}

void CSiteView::OnFileOpen()
{
	// TODO: Add your command handler code here
}

void CSiteView::OnFileOpenWith()
{
	// TODO: Add your command handler code here
}

void CSiteView::OnDummyCompile()
{
	// TODO: Add your command handler code here
}

void CSiteView::OnEditCut()
{
	// TODO: Add your command handler code here
}

void CSiteView::OnEditCopy()
{
	// TODO: Add your command handler code here
}

void CSiteView::OnEditClear()
{
	// TODO: Add your command handler code here
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

	m_FileViewImages.Create(24, bmpObj.bmHeight, nFlags, 0, 0);
	m_FileViewImages.Add(&bmp, RGB(255, 255, 255));

	m_wndSiteView.SetImageList(&m_FileViewImages, TVSIL_NORMAL);
}


