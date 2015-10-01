
// HTRecorderManagerView.cpp : implementation of the CHTRecorderManagerView class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "HTRecorderManager.h"
#endif

#include "HTRecorderManagerDoc.h"
#include "HTRecorderManagerView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CHTRecorderManagerView

IMPLEMENT_DYNCREATE(CHTRecorderManagerView, CView)

BEGIN_MESSAGE_MAP(CHTRecorderManagerView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CHTRecorderManagerView::OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

// CHTRecorderManagerView construction/destruction

CHTRecorderManagerView::CHTRecorderManagerView()
{
	// TODO: add construction code here

}

CHTRecorderManagerView::~CHTRecorderManagerView()
{
}

BOOL CHTRecorderManagerView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CHTRecorderManagerView drawing

void CHTRecorderManagerView::OnDraw(CDC* /*pDC*/)
{
	CHTRecorderManagerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
}


// CHTRecorderManagerView printing


void CHTRecorderManagerView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

BOOL CHTRecorderManagerView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CHTRecorderManagerView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CHTRecorderManagerView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

void CHTRecorderManagerView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CHTRecorderManagerView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}


// CHTRecorderManagerView diagnostics

#ifdef _DEBUG
void CHTRecorderManagerView::AssertValid() const
{
	CView::AssertValid();
}

void CHTRecorderManagerView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CHTRecorderManagerDoc* CHTRecorderManagerView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CHTRecorderManagerDoc)));
	return (CHTRecorderManagerDoc*)m_pDocument;
}
#endif //_DEBUG


// CHTRecorderManagerView message handlers
