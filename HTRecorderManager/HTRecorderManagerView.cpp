
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
#include "HTRecorderFactory.h"
#include "HTNotificationReceiverFactory.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#define MAX_LIVE_CHANNEL 1

// CHTRecorderManagerView

IMPLEMENT_DYNCREATE(CHTRecorderManagerView, CFormView)

BEGIN_MESSAGE_MAP(CHTRecorderManagerView, CFormView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CFormView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CFormView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CHTRecorderManagerView::OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_WM_DESTROY()
	ON_WM_SIZE()
END_MESSAGE_MAP()

// CHTRecorderManagerView construction/destruction

CHTRecorderManagerView::CHTRecorderManagerView()
	: CFormView(CHTRecorderManagerView::IDD)
	, m_pVideoView(NULL)
	, m_pD3DLine(NULL)
{
	// TODO: add construction code here
}

CHTRecorderManagerView::~CHTRecorderManagerView()
{
}


BOOL CHTRecorderManagerView::StartRelay(CString strRecorderUuid, CString strRecorderAddress, CString strRecorderUsername, CString strRecorderPassword, CString strCameraUuid, unsigned char * key, size_t nChannel)
{
	HTRecorderFactory::GetInstance().KillRelayStream();
	return HTRecorderFactory::GetInstance().StartRelay( &(HTNotificationReceiverFactory::GetInstance()), 
														strRecorderUuid, 
														strRecorderAddress, 
														strRecorderUsername, 
														strRecorderPassword, 
														strCameraUuid, 
														m_pVideoView, 
														key, 
														nChannel );
}

BOOL CHTRecorderManagerView::StopRelay(CString strRecorderUuid, CString strRecorderAddress, CString strRecorderUsername, CString strRecorderPassword, CString strCameraUuid)
{
	return HTRecorderFactory::GetInstance().StopRelay( &(HTNotificationReceiverFactory::GetInstance()), 
													   strRecorderUuid, 
													   strRecorderAddress, 
													   strRecorderUsername, 
													   strRecorderPassword, 
													   strCameraUuid );
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

HRESULT WINAPI CHTRecorderManagerView::OnD3DDeviceCreated(LPDIRECT3DDEVICE9 pd3dDevice, LPD3DXSPRITE pd3dSprite, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
	CHTRecorderManagerView * pCatDlg = reinterpret_cast<CHTRecorderManagerView*>(pUserContext);
	pCatDlg->OnDxDeviceCreated(pd3dDevice, pd3dSprite, pBackBufferSurfaceDesc);

	return S_OK;
}

HRESULT WINAPI CHTRecorderManagerView::OnD3DDeviceDestroyed(void *pUserContext)
{
	CHTRecorderManagerView * pCatDlg = reinterpret_cast<CHTRecorderManagerView*>(pUserContext);
	pCatDlg->OnDxDeviceDestroyed();

	return S_OK;
}

void WINAPI CHTRecorderManagerView::OnD3DDeviceLost(void *pUserContext)
{
	CHTRecorderManagerView * pCatDlg = reinterpret_cast<CHTRecorderManagerView*>(pUserContext);
	pCatDlg->OnDxDeviceLost();
}

void WINAPI CHTRecorderManagerView::OnD3DDeviceReset(LPDIRECT3DDEVICE9 pd3dDevice, LPD3DXSPRITE pd3dSprite, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
	CHTRecorderManagerView * pCatDlg = reinterpret_cast<CHTRecorderManagerView*>(pUserContext);
	pCatDlg->OnDxDeviceReset(pd3dDevice, pd3dSprite, pBackBufferSurfaceDesc);
}

void WINAPI CHTRecorderManagerView::OnD3D9CustomDraw(DISP_DX3D9* pDisp)
{
	CHTRecorderManagerView * pCatDlg = reinterpret_cast<CHTRecorderManagerView*>(pDisp->pUserContext);
	if (pCatDlg){
		pCatDlg->OnDxCustomDraw(pDisp);
	}
}

void WINAPI CHTRecorderManagerView::OnD3DPannelEvent(UINT nID, UINT nEvent, DX3DUTIL::CDX3DBaseControl* pControl, void* pUserContext)
{
	CHTRecorderManagerView * pCatDlg = reinterpret_cast<CHTRecorderManagerView*>(pUserContext);
	pCatDlg->OnDxPannelEvent(nID, nEvent, pControl);
}

BOOL WINAPI CHTRecorderManagerView::OnDispMsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, void* pUserContext)
{
	if (!pUserContext)
		return FALSE;

	CHTRecorderManagerView * pCatDlg = reinterpret_cast<CHTRecorderManagerView*>(pUserContext);

	return pCatDlg->OnDxDispMsgProc(hWnd, uMsg, wParam, lParam);
}

void CHTRecorderManagerView::OnDxDeviceCreated(LPDIRECT3DDEVICE9 pd3dDevice, LPD3DXSPRITE pd3dSprite, const D3DSURFACE_DESC* pBackBufferSurfaceDesc)
{
	m_dx3dResource.OnD3D9CreateDevice(pd3dDevice, pd3dSprite);

	if (m_pD3DLine == NULL)
		D3DXCreateLine(pd3dDevice, &m_pD3DLine);
}

void CHTRecorderManagerView::OnDxDeviceDestroyed()
{
	if (m_pD3DLine)
	{
		m_pD3DLine->Release();
		m_pD3DLine = NULL;
	}
	m_dx3dResource.OnD3D9DestroyDevice();
}

void CHTRecorderManagerView::OnDxDeviceLost()
{
	if (m_pD3DLine)
		m_pD3DLine->OnLostDevice();
	m_dx3dResource.OnD3D9LostDevice();
}

void CHTRecorderManagerView::OnDxDeviceReset(LPDIRECT3DDEVICE9 pd3dDevice, LPD3DXSPRITE pd3dSprite, const D3DSURFACE_DESC* pBackBufferSurfaceDesc)
{
	if (m_pD3DLine)
		m_pD3DLine->OnResetDevice();
	m_dx3dResource.OnD3D9ResetDevice(pd3dDevice, pd3dSprite);
}

void CHTRecorderManagerView::OnDxCustomDraw(DISP_DX3D9* pDisp)
{
	CHTRecorderManagerView * pCatDlg = reinterpret_cast<CHTRecorderManagerView*>(pDisp->pUserContext);
	CString strCh = _T("");

	pCatDlg->m_dx3dResource.Lock();
	if (pCatDlg->m_dx3dResource.GetD3D9Device())
	{
		for (int i = 0; i<MAX_LIVE_CHANNEL; i++)
		{
			CRect rt;
			pCatDlg->m_pVideoView->GetRect2(i, &rt);
			strCh.Format(L"Ch %d", i);

			if ((rt.left == 0) && (rt.top == 0) && (rt.right == 0) && (rt.bottom == 0))
			{
				// Display에 없는 부분
				pCatDlg->m_dx3dResource.m_pd3dSprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE);
				pCatDlg->m_dx3dResource.GetFont(0)->pFont->DrawText(pCatDlg->m_dx3dResource.m_pd3dSprite, _T(""), -1, CRect(0, 0, 0, 0), DT_NOCLIP, D3DCOLOR_XRGB(0, 0, 0));
				pCatDlg->m_dx3dResource.m_pd3dSprite->End();
			}
			else
			{
				// Display에 있는 부분
				pCatDlg->m_dx3dResource.m_pd3dSprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE);
				pCatDlg->m_dx3dResource.GetFont(0)->pFont->DrawText(pCatDlg->m_dx3dResource.m_pd3dSprite, strCh, -1, CRect(rt.left + 10, rt.top + 10, 0, 0), DT_NOCLIP, D3DCOLOR_XRGB(255, 255, 255));
				/*
				pCatDlg->m_dx3dResource.GetFont(0)->pFont->DrawText(pCatDlg->m_dx3dResource.m_pd3dSprite, _T("CameraName"), -1, CRect(rt.left+10, rt.top+25, 0, 0), DT_NOCLIP, D3DCOLOR_XRGB(255, 255, 255));
				*/
				pCatDlg->m_dx3dResource.m_pd3dSprite->End();

			}
		}
	}
	pCatDlg->m_dx3dResource.Unlock();

}

void CHTRecorderManagerView::OnDxPannelEvent(UINT nID, UINT nEvent, DX3DUTIL::CDX3DBaseControl* pControl)
{

}

BOOL CHTRecorderManagerView::OnDxDispMsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		m_dx3dResource.AddFont(L"Arial", 15, FW_BOLD, FALSE);
		break;
	default:
		break;
	}
	return FALSE;
}

D3DTLVERTEX CHTRecorderManagerView::CreateD3DTLVERTEX(float X, float Y, float Z, float RHW, D3DCOLOR color, float U, float V)
{
	D3DTLVERTEX v;
	v.fX = X;
	v.fY = Y;
	v.fZ = Z;
	v.fRHW = RHW;
	v.Color = color;
	v.fU = U;
	v.fV = V;

	return v;
}    //CreateD3DTLVERTEX

void CHTRecorderManagerView::DrawCircle(CPoint pt, float fRadius, D3DCOLOR color, LPDIRECT3DDEVICE9 lp3dDevice)
{
	D3DTLVERTEX d3dvTex[400];

	float x1 = pt.x;
	float y1 = pt.y;

	lp3dDevice->SetTexture(0, NULL);

	for (int i = 0; i <= 363; i += 3)
	{
		float angle = (i / 50.3f);   // 57.3f
		float x2 = pt.x + (fRadius * sin(angle));
		float y2 = pt.y + (fRadius * cos(angle));

		d3dvTex[i] = CreateD3DTLVERTEX(pt.x, pt.y, 0, 1, color, 0, 0);
		d3dvTex[i + 1] = CreateD3DTLVERTEX(x1, y1, 0, 1, color, 0, 0);
		d3dvTex[i + 2] = CreateD3DTLVERTEX(x2, y2, 0, 1, color, 0, 0);

		y1 = y2;
		x1 = x2;
	}

	lp3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 363, d3dvTex, sizeof(D3DTLVERTEX));
	//lp3dDevice->SetStreamSource(0, m_vb, 0, sizeof(CUSTOMVERTEX));
	//lp3dDevice->SetFVF( D3DFVF_CUSTOMVERTEX );

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

void CHTRecorderManagerView::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();

	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	m_pVideoView = CDisplayLib::CreateDisplayLib();

	m_pVideoView->SetUserContext(this);
	//m_pVideoView->SetBackgroundColor(RGB(0x00, 0x00, 0x00));
	//D3DCOLOR color;
	//m_pVideoView->SetChannelBackground(0, D3DCOLOR_ARGB(0x00, 0x00, 0x00, 0x00));
	m_pVideoView->SetCallbackD3D9DeviceCreated(OnD3DDeviceCreated);
	m_pVideoView->SetCallbackD3D9DeviceDestroyed(OnD3DDeviceDestroyed);
	m_pVideoView->SetCallbackD3D9CustomDraw(OnD3D9CustomDraw);
	m_pVideoView->SetCallbackD3D9DeviceReset(OnD3DDeviceReset);
	m_pVideoView->SetCallbackD3D9DeviceLost(OnD3DDeviceLost);
	m_pVideoView->SetCallbackDispMsgProc(OnDispMsgProc);

	m_pVideoView->Create(MAX_LIVE_CHANNEL, WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN, CRect(0, 0, 0, 0), NULL, GetSafeHwnd());
	m_pVideoView->SetViewLayout(VIEW1x1);

	RECT rectVideoView;
	GetWindowRect(&rectVideoView);
	m_pVideoView->SetWindowSize(rectVideoView.right, rectVideoView.bottom);
}

void CHTRecorderManagerView::OnDestroy()
{
	CFormView::OnDestroy();

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if (m_pVideoView)
	{
		m_pVideoView->Destroy();
		CDisplayLib::DestroyDisplayLib(m_pVideoView);
		m_pVideoView = NULL;
	}
}

void CHTRecorderManagerView::OnSize(UINT nType, int cx, int cy)
{
	CFormView::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if (m_pVideoView && m_pVideoView->m_hWnd)
	{
		m_pVideoView->SetWindowSize(cx, cy);

		//GetDlgItem(IDC_STATIC_1)->MoveWindow(10, cy - 140, 60, 16);
		//GetDlgItem(IDC_STATIC_2)->MoveWindow(10, cy - 110, 60, 16);
		//GetDlgItem(IDC_STATIC_3)->MoveWindow(10, cy - 80, 60, 16);

		//GetDlgItem(IDC_EDIT_NRS_IP)->MoveWindow(70, cy - 140, 180, 23);
		//GetDlgItem(IDC_EDIT_USER_ID)->MoveWindow(70, cy - 110, 100, 23);
		//GetDlgItem(IDC_EDIT_USER_PW)->MoveWindow(70, cy - 80, 100, 23);
		//GetDlgItem(IDC_BUTTON_NRS_CONNECT)->MoveWindow(180, cy - 110, 105, 50);
		//GetDlgItem(IDC_BUTTON_CAMERA_REGISTER)->MoveWindow(10, cy - 45, 130, 40);
		//GetDlgItem(IDC_LIST_LOG)->MoveWindow(300, cy - 150, 300, 150);
		//GetDlgItem(IDC_EDIT_STATUS)->MoveWindow(600, cy - 150, cx - 600, 150);
		//GetDlgItem(IDC_BTN_PLAYBACK_VIEW)->MoveWindow(155, cy - 45, 130, 40);

		Invalidate();
	}
}
