
// HTRecorderManagerView.h : interface of the CHTRecorderManagerView class
//

#pragma once
#include "D3DRender.h"
#include "DisplayLib.h"
#include "dx3d_gui.h"
#include "dx3d_gui_button.h"
#include "dx3d_gui_static.h"

typedef struct{
	float fX;
	float fY;
	float fZ;
	float fRHW;
	D3DCOLOR Color;
	float fU;
	float fV;
}D3DTLVERTEX;

class CHTRecorderManagerView : public CFormView
{
protected: // create from serialization only
	CHTRecorderManagerView();
	DECLARE_DYNCREATE(CHTRecorderManagerView)

	// 대화 상자 데이터입니다.
	enum { IDD = IDD_DISPLAY_FORMVIEW };
// Attributes
public:
	CHTRecorderManagerDoc* GetDocument() const;

	BOOL StartRelay(CString strRecorderUuid, CString strRecorderAddress, CString strRecorderUsername, CString strRecorderPassword, CString strCameraUuid, unsigned char * key, size_t nChannel);
	BOOL StopRelay(CString strRecorderUuid, CString strRecorderAddress, CString strRecorderUsername, CString strRecorderPassword, CString strCameraUuid);

private:
	CDisplayLib * m_pVideoView;
	LPD3DXLINE m_pD3DLine;
	DX3DUTIL::CDX3DResourceManager m_dx3dResource;

private:
	/////////////////////////////////////////////////////////////////////////////////////////////////
	// DIrectX3D 관련
	virtual void	OnDxDeviceCreated(LPDIRECT3DDEVICE9 pd3dDevice, LPD3DXSPRITE pd3dSprite, const D3DSURFACE_DESC* pBackBufferSurfaceDesc);
	virtual void	OnDxDeviceDestroyed();
	virtual void	OnDxDeviceLost();
	virtual void	OnDxDeviceReset(LPDIRECT3DDEVICE9 pd3dDevice, LPD3DXSPRITE pd3dSprite, const D3DSURFACE_DESC* pBackBufferSurfaceDesc);
	virtual void	OnDxCustomDraw(DISP_DX3D9* pDisp);
	virtual void	OnDxPannelEvent(UINT nID, UINT nEvent, DX3DUTIL::CDX3DBaseControl* pControl);
	virtual BOOL	OnDxDispMsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	/////////////////////////////////////////////////////////////////////////////////////////////////
	// DIrectX3D 콜백함수
	static HRESULT WINAPI OnD3DDeviceCreated(LPDIRECT3DDEVICE9 pd3dDevice, LPD3DXSPRITE pd3dSprite, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
	static HRESULT WINAPI OnD3DDeviceDestroyed(void *pUserContext);
	static void WINAPI OnD3DDeviceLost(void *pUserContext);
	static void WINAPI OnD3DDeviceReset(LPDIRECT3DDEVICE9 pd3dDevice, LPD3DXSPRITE pd3dSprite, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
	static void WINAPI OnD3D9CustomDraw(DISP_DX3D9* pDisp);
	static void WINAPI OnD3DPannelEvent(UINT nID, UINT nEvent, DX3DUTIL::CDX3DBaseControl* pControl, void* pUserContext);
	static BOOL WINAPI OnDispMsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, void* pUserContext);

	void DrawCircle(CPoint pt, float fRadius, D3DCOLOR color, LPDIRECT3DDEVICE9 lp3dDevice);
	D3DTLVERTEX CreateD3DTLVERTEX(float X, float Y, float Z, float RHW, D3DCOLOR color, float U, float V);
// Operations
public:

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CHTRecorderManagerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDestroy();
	virtual void OnInitialUpdate();
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

#ifndef _DEBUG  // debug version in HTRecorderManagerView.cpp
inline CHTRecorderManagerDoc* CHTRecorderManagerView::GetDocument() const
   { return reinterpret_cast<CHTRecorderManagerDoc*>(m_pDocument); }
#endif

