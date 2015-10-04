#ifndef _DX3D_GUI_H
#define _DX3D_GUI_H
#pragma comment(lib, "D3D9.lib")
#pragma comment(lib, "D3dx9.lib")
#include <D3D9.h>
#include <D3DX9.h>
#include "GrowableArray.hpp"
#include <string>
#include <list>
#include <map>
#include <memory>

#define DX3DUTIL_SAFE_RELEASE(p) if(p) { p->Release(); p=NULL; }
namespace DX3DUTIL {
	class CDX3DResourceManager;
	class CDX3DPannel;
	class CDX3DBaseControl;
	class CDX3DStatic;
	class CDX3DButton;
};

typedef void (CALLBACK*	LPDX3DPANNELEVENTCALLBACK)(UINT nID, UINT nEvent, DX3DUTIL::CDX3DBaseControl* pControl, void* pUserContext);

namespace DX3DUTIL
{

enum DXGUI_EVENT {
	DXGUI_EVENT_BUTTON_CLICKED		= 0x1000,
	
	DXGUI_EVENT_LBUTTONDOWN,
	DXGUI_EVENT_LBUTTONDBLCLICK,
	DXGUI_EVENT_LBUTTONUP,
	DXGUI_EVENT_RBUTTONDOWN,
	DXGUI_EVENT_RBUTTONDBLCLICK,
	DXGUI_EVENT_RBUTTONUP,

	DXGUI_EVENT_MOUSEENTER,
	DXGUI_EVENT_MOUSELEAVE,

	DXGUI_EVENT_REDRAWCONTROL,

	DXGUI_EVENT_MAX,
};

///////////////////////////////////////////////////////////////////////////////////////
class CDX3DResourceManager
{
public:
	typedef struct _DX3D9Texture
	{
		_DX3D9Texture() : bIsFile(FALSE), hModule(NULL), dwWidth(0), dwHeight(0), crColorKey(0), pTexture(NULL), pSurface(NULL), nStreamID(-1), pStream(NULL), nLength(0)
		{			
		};
		BOOL bIsFile;
		HMODULE hModule;
		UINT nResourceID;
		CString strFileName;
		CString	strResourceType;
		int   nStreamID; // Image Stream ID
		char* pStream;	// Image Stream
		int	  nLength;	// Image Length
		DWORD dwWidth;
		DWORD dwHeight;
		D3DCOLOR crColorKey;
		LPDIRECT3DTEXTURE9 pTexture;
		LPDIRECT3DSURFACE9 pSurface;
	} DX3D9Texture;

	typedef struct _DX3D9Font
	{
		_DX3D9Font() : pFont(NULL), nHeight(0), nWeight(0), bItalic(FALSE)
		{}
		CString	strFontName;
		LONG nHeight;
		LONG nWeight;
		BOOL bItalic;
		ID3DXFont*	pFont;
	} DX3D9Font;

	CDX3DResourceManager();
	virtual ~CDX3DResourceManager();

	void	Lock() { EnterCriticalSection(&m_cs); }
	void	Unlock() { LeaveCriticalSection(&m_cs); }

	void	OnD3D9CreateDevice(LPDIRECT3DDEVICE9 pd3dDevice, LPD3DXSPRITE pd3dSprite);
	void	OnD3D9ResetDevice(LPDIRECT3DDEVICE9 pd3dDevice, LPD3DXSPRITE pd3dSprite);
	void	OnD3D9LostDevice();
	void	OnD3D9DestroyDevice();
	LPDIRECT3DDEVICE9	GetD3D9Device() { return m_pd3dDevice; }

	int AddFont(const CString& FontName, LONG nHeight, LONG nWeight, BOOL bItalic = FALSE);
	int AddTexture(LPCTSTR lpFileName, D3DCOLOR crColorKey = D3DCOLOR_ARGB(0,0,0,0));
	int AddTexture(UINT nResourceID, LPCTSTR lpResourceType, HMODULE hResourceModule, D3DCOLOR crColorKey = D3DCOLOR_ARGB(0,0,0,0));
	int AddTexture(int nStreamID, int nLength, LPVOID pStream, D3DCOLOR crColorKey = D3DCOLOR_ARGB(0,0,0,0));

	DX3D9Font*		GetFont(int nFont);
	DX3D9Texture*	GetTexture(int nTexture);

	LPD3DXSPRITE		m_pd3dSprite;

protected:
	BOOL	CreateTexture(int nTexture);
	BOOL	CreateFont(int nFont);
	LPDIRECT3DDEVICE9	m_pd3dDevice;

	CGrowableArray<DX3D9Font*>		m_FontArray;
	CGrowableArray<DX3D9Texture*>	m_TextureArray;

	CRITICAL_SECTION	m_cs;

	friend class CDX3DBaseControl;
	friend class CDX3DPannel;
};

///////////////////////////////////////////////////////////////////////////////////////
class CDX3DPannel
{
public:
	CDX3DPannel(CDX3DResourceManager& Resource, HWND hParent, void* pUserContext = NULL);
	virtual ~CDX3DPannel();

	virtual void		OnRender(float fElapsedTime);
	virtual BOOL		MsgProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);

	static void			ResetStaticControl();

	void				SetCallbackEvent(LPDX3DPANNELEVENTCALLBACK pCallbackEvent) { m_CallbackEvent = pCallbackEvent; }
	void				SetID(int nID) { m_nID = nID; }
	int					GetID() const { return m_nID; }
	void				SetEnabled(BOOL bEnabled) { m_bEnabled = bEnabled; }
	BOOL				GetEnabled() const { return m_bEnabled; }
	void				SetVisible(BOOL bVisible) { m_bVisible = bVisible; }
	BOOL				GetVisible() const { return m_bVisible; }
	void				SetLocation(int x, int y);
	void				SetSize(int width, int height);
	void				SetRect(CRect& Rect);
	void				GetRect(CRect& Rect);
	BOOL				PtInRect(POINT pt) { return ::PtInRect(&m_rtPannel, pt); }
	void				SetResourceFont(int nFontID);
	CDX3DResourceManager*	GetResource();

	void				SetParentHwnd(HWND hWnd) { m_hParentWnd = hWnd; }
	HWND				GetParentHwnd() const { return m_hParentWnd; }
	CDX3DBaseControl*	AddControl(std::tr1::shared_ptr<CDX3DBaseControl> pControl);
	CDX3DStatic*		AddStatic(size_t nID, LPCTSTR lpText, int nTexture, const CRect& rtTexture, const CRect& rtLocation);
	CDX3DButton*		AddButton(size_t nID, LPCTSTR lpText, int nTexture, const CRect* prtTextures, int nTextureCount, const CRect& rtLocation);

	CDX3DBaseControl*	GetPtInControl(POINT pt);

	void				SetFocusControl(CDX3DBaseControl* pControl);
	void				SendEvent(UINT nEvent, CDX3DBaseControl* pControl);
	void				DrawControl(CDX3DBaseControl* pControl);

protected:
	CDX3DResourceManager&	m_Resource;
	std::list<std::tr1::shared_ptr<CDX3DBaseControl>> m_ControlList;
	virtual void		UpdateRects();
	virtual void		OnMouseMove(POINT pt, CDX3DBaseControl* pControl = NULL);
	virtual void		OnMouseUp(POINT pt);

	int		m_nID;
	void*	m_pUserContext;
	BOOL	m_bEnabled;
	BOOL	m_bVisible;
	int		m_x, m_y;
	int		m_width, m_height;
	CRect	m_rtPannel;

	int		m_nFontID;

	static CDX3DBaseControl*	m_pControlFocus;
	static CDX3DBaseControl*	m_pControlMouseOver;

	HWND	m_hParentWnd;
	LPDX3DPANNELEVENTCALLBACK	m_CallbackEvent;

	friend class CDX3DBaseControl;
};

///////////////////////////////////////////////////////////////////////////////////////
class CDX3DBaseControl
{
public:
	enum CONTROL_TYPE {
		CONTROL_STATIC = 0,
		CONTROL_BUTTON,
		CONTROL_MAX,
	};
	CDX3DBaseControl(CDX3DPannel* pDxPannel);
	virtual ~CDX3DBaseControl();

	virtual void	Refresh();
	virtual void	Render(float fElapsedTime) {}

	virtual BOOL	MsgProc(UINT nMsg, WPARAM wParam, LPARAM lParam) { return FALSE; }
	virtual BOOL	HandleMouse(UINT nMsg, POINT pt, WPARAM wParam, LPARAM lParam) { return FALSE; }
	
	virtual void	OnFocusIn() { m_bHasFocus = TRUE; }
	virtual void	OnFocusOut() { m_bHasFocus = FALSE; }
	virtual void	OnMouseEnter() { m_bMouseOver = TRUE; }
	virtual void	OnMouseLeave() { m_bMouseOver = FALSE; }
	virtual BOOL	ContainsPoint(POINT pt) { return ::PtInRect(&m_rcBoundingBox, pt); }
	virtual void	SetEnabled(BOOL bEnabled) { m_bEnabled = bEnabled; }
	virtual BOOL	GetEnabled() { return m_bEnabled; }
	virtual void	SetVisible(BOOL bVisible) { m_bVisible = bVisible; }
	virtual BOOL	GetVisible() { return m_bVisible; }
	virtual BOOL	GetHasFocus() const { return FALSE; }
	virtual void	SetControlType(CONTROL_TYPE Type) { m_ControlType = Type; }
	virtual CONTROL_TYPE GetControlType() const { return m_ControlType; }
	virtual void	SetToolTipText(LPCTSTR lpText) {}
	
	void			SetID(size_t nID) { m_nID = nID; }
	size_t			GetID() const { return m_nID; }
	void			SetLocation(int x, int y);
	void			SetSize(int width, int height);
	void			SetRect(CRect &rect);
	CRect			GetRect() const;
	void			SetUserData(void* pUserData) { m_pUserData = pUserData; }
	void*			GetUserData() const { return m_pUserData; }

	void			SetDxPannel(CDX3DPannel* pDxPannel) { m_pDxPannel = pDxPannel; }
	CDX3DPannel*	GetDxPannel() { return m_pDxPannel; }

	void			SetResourceFont(int nFont) { m_nResourceFont = nFont; }
	int				GetResourceFont() const { return m_nResourceFont; }
	void			SetResourceTexture(int nTexture) { m_nResourceTexture = nTexture; }
	int				GetResourceTexture() const { return m_nResourceTexture; }
	void			SetTextureRect(const RECT& rtTexture) { m_rtTexture = rtTexture; }
	CRect			GetTextureRect() const { return m_rtTexture; }

protected:
	virtual void	UpdateRects();
	size_t	m_nID;
	BOOL	m_bEnabled;
	BOOL	m_bVisible;
	BOOL	m_bHasFocus;
	BOOL	m_bMouseOver;
	int		m_x, m_y;
	int		m_width, m_height;
	RECT	m_rcBoundingBox;
	void*	m_pUserData;
	CDX3DPannel*	m_pDxPannel;

	// CDX3DResourceManager에서 관리하는 Font, Texture ID
	int		m_nResourceFont;
	int		m_nResourceTexture;
	// Texture에서 그려질 이미지의 RECT
	CRect	m_rtTexture;

	CONTROL_TYPE	m_ControlType;

	friend class CDX3DPannel;
};

};

#endif // _DX3D_GUI_H