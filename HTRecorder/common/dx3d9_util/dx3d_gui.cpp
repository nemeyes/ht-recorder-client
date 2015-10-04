#include "StdAfx.h"
#include "dx3d_gui.h"
#include <d3dx9tex.h>

#include "dx3d_gui_static.h"
#include "dx3d_gui_button.h"

using namespace DX3DUTIL;

///////////////////////////////////////////////////////////////////////////////////////
// CDX3DResourceManager
CDX3DResourceManager::CDX3DResourceManager()
: m_pd3dDevice(NULL)
, m_pd3dSprite(NULL)
{
	InitializeCriticalSection(&m_cs);
}

CDX3DResourceManager::~CDX3DResourceManager()
{
	for(int i=0; i<m_TextureArray.GetSize(); i++)
	{
		DX3D9Texture* pTexture = m_TextureArray.GetAt(i);
		if (pTexture->pStream)
			delete pTexture->pStream;
		DX3DUTIL_SAFE_RELEASE(pTexture->pTexture);
		DX3DUTIL_SAFE_RELEASE(pTexture->pSurface);
		delete pTexture;
	}
	m_TextureArray.RemoveAll();

	for(int i=0; i<m_FontArray.GetSize(); i++)
	{
		DX3D9Font* pFont = m_FontArray.GetAt(i);
		DX3DUTIL_SAFE_RELEASE(pFont->pFont);
		delete pFont;
	}
	m_FontArray.RemoveAll();
	DeleteCriticalSection(&m_cs);
}

void CDX3DResourceManager::OnD3D9CreateDevice(LPDIRECT3DDEVICE9 pd3dDevice, LPD3DXSPRITE pd3dSprite)
{
	Lock();
	m_pd3dDevice = pd3dDevice;
	m_pd3dSprite = pd3dSprite;

	for(int i=0; i<m_TextureArray.GetSize(); i++)
		CreateTexture(i);
	for(int i=0; i<m_FontArray.GetSize(); i++)
		CreateFont(i);

	Unlock();
}

void CDX3DResourceManager::OnD3D9ResetDevice(LPDIRECT3DDEVICE9 pd3dDevice, LPD3DXSPRITE pd3dSprite)
{
	Lock();
	m_pd3dDevice = pd3dDevice;
	m_pd3dSprite = pd3dSprite;
	
	DX3D9Font*		pFont;
	for(int i=0; i<m_TextureArray.GetSize(); i++)
		CreateTexture(i);

	for(int i=0; i<m_FontArray.GetSize(); i++)
	{
		pFont = m_FontArray.GetAt(i);
		if(pFont->pFont)
			pFont->pFont->OnResetDevice();

		CreateFont(i);
	}

	Unlock();
}

void CDX3DResourceManager::OnD3D9LostDevice()
{
	Lock();

	m_pd3dSprite->OnLostDevice();

	DX3D9Font* pFont;
	for(int i=0; i<m_FontArray.GetSize(); i++)
	{
		 pFont = m_FontArray.GetAt(i);
		 if(pFont->pFont)
			 pFont->pFont->OnLostDevice();
	}
	Unlock();
}

void CDX3DResourceManager::OnD3D9DestroyDevice()
{
	Lock();
	m_pd3dDevice = NULL;
	m_pd3dSprite = NULL;
	
	DX3D9Texture*	pTexture;
	DX3D9Font*		pFont;
	for(int i=0; i<m_TextureArray.GetSize(); i++)
	{
		pTexture = m_TextureArray.GetAt(i);
		DX3DUTIL_SAFE_RELEASE(pTexture->pTexture);
		DX3DUTIL_SAFE_RELEASE(pTexture->pSurface);
	}
	for(int i=0; i<m_FontArray.GetSize(); i++)
	{
		pFont = m_FontArray.GetAt(i);
		DX3DUTIL_SAFE_RELEASE(pFont->pFont);
	}

	Unlock();
}

int CDX3DResourceManager::AddFont(const CString& FontName, LONG nHeight, LONG nWeight, BOOL bItalic /*= FALSE*/)
{
#if 0
	for(int i=0; i<m_FontArray.GetSize(); i++)
	{
		DX3D9Font* pFont = m_FontArray.GetAt(i);
		if(pFont->strFontName == FontName && pFont->nHeight == nHeight && pFont->nWeight == nWeight)
			return i;
	}
#endif
	// 같은 이름의 폰트가 있으면 다시 생성하도록
	for(int i=0; i<m_FontArray.GetSize(); i++)
	{
		DX3D9Font* pFont = m_FontArray.GetAt(i);
		if(pFont->strFontName == FontName)
		{
			if(pFont->nHeight == nHeight && pFont->nWeight == nWeight && pFont->bItalic == bItalic)
				return i;

			// 크기가 다르다면 다시 할당
			
			pFont->strFontName	= FontName;
			pFont->nHeight		= nHeight;
			pFont->nWeight		= nWeight;
			pFont->bItalic		= bItalic;
			if(m_pd3dDevice)
				CreateFont(i);

			return i;
		}
	}

	DX3D9Font* pFont	= new DX3D9Font;
	pFont->strFontName	= FontName;
	pFont->nHeight		= nHeight;
	pFont->nWeight		= nWeight;
	pFont->bItalic		= bItalic;

	m_FontArray.Add(pFont);

	int nFont = m_FontArray.GetSize() - 1;
	if(m_pd3dDevice)
		CreateFont(nFont);

	return nFont;
}

int CDX3DResourceManager::AddTexture(int nStreamID, int nLength, LPVOID pStream, D3DCOLOR crColorKey /*= D3DCOLOR_ARGB(0,0,0,0)*/)
{
	for(int i=0; i<m_TextureArray.GetSize(); i++)
	{
		DX3D9Texture* pTexture = m_TextureArray.GetAt(i);
		if(pTexture->nStreamID == nStreamID)
			return i;
	}

	DX3D9Texture* pTexture = new DX3D9Texture;

	pTexture->crColorKey	= crColorKey;
	pTexture->nStreamID		= nStreamID;
	pTexture->pStream		= new char[nLength];
	pTexture->nLength		= nLength;

	memcpy(pTexture->pStream, pStream, nLength);

	m_TextureArray.Add( pTexture );
	int nTexture = m_TextureArray.GetSize() - 1;

	if(m_pd3dDevice)
		CreateTexture( nTexture );

	return nTexture;
}


int CDX3DResourceManager::AddTexture(LPCTSTR lpFileName, D3DCOLOR crColorKey /*= D3DCOLOR_ARGB(0,0,0,0)*/)
{
	for(int i=0; i<m_TextureArray.GetSize(); i++)
	{
		DX3D9Texture* pTexture = m_TextureArray.GetAt(i);
		if(pTexture->strFileName == lpFileName)
			return i;
	}

	DX3D9Texture* pTexture = new DX3D9Texture;

	pTexture->bIsFile		= TRUE;
	pTexture->crColorKey	= crColorKey;
	pTexture->strFileName	= lpFileName;

	m_TextureArray.Add( pTexture );
	int nTexture = m_TextureArray.GetSize() - 1;

	if(m_pd3dDevice)
		CreateTexture( nTexture );

	return nTexture;
}

int CDX3DResourceManager::AddTexture(UINT nResourceID, LPCTSTR lpResourceType, HMODULE hResourceModule, D3DCOLOR crColorKey /*= D3DCOLOR_ARGB(0,0,0,0)*/)
{
	CString strKey;
	strKey.Format(_T("%d"), nResourceID);

	for(int i=0; i<m_TextureArray.GetSize(); i++)
	{
		DX3D9Texture* pTexture = m_TextureArray.GetAt(i);
		if(pTexture->strFileName == strKey)
			return i;
	}

	DX3D9Texture* pTexture = new DX3D9Texture;

	pTexture->hModule		= hResourceModule;
	pTexture->crColorKey	= crColorKey;
	pTexture->strFileName		= strKey;
	pTexture->nResourceID		= nResourceID;
	pTexture->strResourceType	= lpResourceType;

	m_TextureArray.Add( pTexture );
	int nTexture = m_TextureArray.GetSize() - 1;

	if(m_pd3dDevice)
		CreateTexture( nTexture );

	return nTexture;
}

BOOL CDX3DResourceManager::CreateTexture(int nTexture)
{
	HRESULT hr;
	DX3D9Texture* pTexture = m_TextureArray.GetAt(nTexture);
	DX3DUTIL_SAFE_RELEASE(pTexture->pSurface);
	DX3DUTIL_SAFE_RELEASE(pTexture->pTexture);

	D3DXIMAGE_INFO Info;
	if(pTexture->bIsFile)
	{
		if(FAILED(D3DXGetImageInfoFromFile(pTexture->strFileName, &Info)))
			return FALSE;

		hr = D3DXCreateTextureFromFileEx(m_pd3dDevice,
			pTexture->strFileName, 
			D3DX_DEFAULT_NONPOW2,	//Info.Width,
			D3DX_DEFAULT_NONPOW2,	//Info.Height,
			D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
			D3DX_DEFAULT,	// D3DX_DEFAULT
			D3DX_DEFAULT,	// D3DX_DEFAULT
			pTexture->crColorKey,
			&Info,
			NULL,
			&pTexture->pTexture);
	}
	else if (pTexture->pStream && pTexture->nLength > 0)
	{
		hr = D3DXCreateTextureFromFileInMemoryEx(m_pd3dDevice,
			pTexture->pStream, pTexture->nLength,
			D3DX_DEFAULT_NONPOW2,
			D3DX_DEFAULT_NONPOW2,
			D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
			D3DX_DEFAULT,
			D3DX_DEFAULT,
			pTexture->crColorKey,
			&Info,
			NULL,
			&pTexture->pTexture);		
	}
	else 
	{
		HMODULE hModule = pTexture->hModule;

		HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(pTexture->nResourceID), pTexture->strResourceType);
		if(!hResource)
			return FALSE;

		HGLOBAL hMem = LoadResource(hModule, hResource);
		DWORD dwSize = SizeofResource(hModule, hResource);

		if(hMem)
		{
			const void* pResourceData = (LPVOID)LockResource(hMem);

			hr = D3DXCreateTextureFromFileInMemoryEx(m_pd3dDevice,
				pResourceData, dwSize,
				D3DX_DEFAULT_NONPOW2,
				D3DX_DEFAULT_NONPOW2,
				D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
				D3DX_DEFAULT,
				D3DX_DEFAULT,
				pTexture->crColorKey,
				&Info,
				NULL,
				&pTexture->pTexture);
			
			UnlockResource(hMem);
		}

		FreeResource(hResource);
		

	/*	hr = D3DXCreateTextureFromResourceEx(m_pd3dDevice,
			pTexture->hModule, (LPCTSTR)pTexture->strFileName,
			D3DX_DEFAULT_NONPOW2,
			D3DX_DEFAULT_NONPOW2,
			D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
			D3DX_DEFAULT,
			D3DX_DEFAULT,
			pTexture->crColorKey,
			&Info,
			NULL,
			&pTexture->pTexture);*/
	}

	if(FAILED(hr))
		return FALSE;

	hr = pTexture->pTexture->GetSurfaceLevel(0, &pTexture->pSurface);
	if(FAILED(hr))
	{
		DX3DUTIL_SAFE_RELEASE(pTexture->pTexture);
		return FALSE;
	}

	pTexture->dwWidth	= Info.Width;
	pTexture->dwHeight	= Info.Height;

	return TRUE;
}

BOOL CDX3DResourceManager::CreateFont(int nFont)
{
	DX3D9Font* pFont = m_FontArray.GetAt(nFont);
	DX3DUTIL_SAFE_RELEASE(pFont->pFont);

	HRESULT hr = D3DXCreateFont( m_pd3dDevice,
		pFont->nHeight,
		0,
		pFont->nWeight,
		1,
		pFont->bItalic,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		pFont->strFontName,
		&pFont->pFont);

	if(FAILED(hr))
		return FALSE;

	return TRUE;
}

CDX3DResourceManager::DX3D9Font* CDX3DResourceManager::GetFont(int nFont)
{
	return m_FontArray.GetAt(nFont);
}

CDX3DResourceManager::DX3D9Texture* CDX3DResourceManager::GetTexture(int nTexture)
{
	return m_TextureArray.GetAt(nTexture);
}

///////////////////////////////////////////////////////////////////////////////////////
// CDX3DPannel

CDX3DBaseControl* CDX3DPannel::m_pControlFocus = NULL;
CDX3DBaseControl* CDX3DPannel::m_pControlMouseOver = NULL;

void CDX3DPannel::ResetStaticControl()
{
	m_pControlFocus = NULL;
	m_pControlMouseOver = NULL;
}

CDX3DPannel::CDX3DPannel(CDX3DResourceManager& Resource, HWND hParent, void* pUserContext /*= NULL*/)
: m_Resource(Resource)
, m_hParentWnd(hParent)
, m_pUserContext(pUserContext)
, m_bEnabled(TRUE)
, m_bVisible(TRUE)
, m_nFontID(-1)
, m_CallbackEvent(NULL)
{
}

CDX3DPannel::~CDX3DPannel()
{
}

void CDX3DPannel::OnRender(float fElapsedTime)
{
	if(!m_bVisible)
		return;

	LPDIRECT3DDEVICE9 pd3dDevice = m_Resource.GetD3D9Device();

	std::list<std::tr1::shared_ptr<CDX3DBaseControl>>::iterator pos = m_ControlList.begin();
	while(pos != m_ControlList.end())
	{
		/*if(m_pControlFocus == pos->get())
		{
			pos++;
			continue;
		}*/

		(*pos)->Render(fElapsedTime);

		pos++;
	}

	/*if(m_pControlFocus)
		m_pControlFocus->Render(fElapsedTime);*/
}

CDX3DBaseControl* CDX3DPannel::AddControl(std::tr1::shared_ptr<CDX3DBaseControl> pControl)
{
	if(pControl == NULL) return NULL;

	m_ControlList.push_back(pControl);
	return pControl.get();
}

CDX3DStatic* CDX3DPannel::AddStatic(size_t nID, LPCTSTR lpText, int nTexture, const CRect& rtTexture, const CRect& rtLocation)
{
	std::tr1::shared_ptr<CDX3DStatic> pStatic(new CDX3DStatic(this));
	pStatic->SetID(nID);
	pStatic->SetText(lpText);
	pStatic->SetLocation(rtLocation.left, rtLocation.top);
	pStatic->SetSize(rtLocation.Width(), rtLocation.Height());
	pStatic->SetResourceTexture(nTexture);
	pStatic->SetTextureRect(rtTexture);

	m_ControlList.push_back(pStatic);

	return pStatic.get();
}

CDX3DButton* CDX3DPannel::AddButton(size_t nID, LPCTSTR lpText, int nTexture, const CRect* prtTextures, int nTextureCount, const CRect& rtLocation)
{
	/*
	버튼이미지의 위치
	텍스쳐 이미지상에서 Height 만큼 밑으로 나열되어있어야 한다!!
	버튼 이미지의 크기는 16x16, 32x32, 64x64 로 되어있어야 함

	버튼1 | 버튼2 |
	노말  | 노말  |
	오버  | 오버  |
	다운  | 다운  |
	흑백  | 흑백  |

	*/
	if(!prtTextures)
		return NULL;

	std::tr1::shared_ptr<CDX3DButton> pButton(new CDX3DButton(this));

	pButton->SetID(nID);
	pButton->SetText(lpText);
	pButton->SetLocation(rtLocation.left, rtLocation.top);
	pButton->SetSize(rtLocation.Width(), rtLocation.Height());
	pButton->SetResourceTexture(nTexture);
	pButton->SetTextureRect(prtTextures[0]);

	int nCount = 0;
	while(nCount < nTextureCount && nCount < CDX3DButton::BUTTON_STATE_MAX)
	{
		pButton->SetButtonTextureRect( (CDX3DButton::BUTTON_STATE)nCount, prtTextures[nCount]);
		nCount++;
	}

	m_ControlList.push_back(pButton);

	return pButton.get();
}

void CDX3DPannel::SetLocation(int x, int y)
{
	m_x = x;
	m_y = y;
	
	UpdateRects();
}

void CDX3DPannel::SetSize(int width, int height)
{
	m_width = width;
	m_height = height;
	
	UpdateRects();
}

void CDX3DPannel::SetRect(CRect& Rect)
{
	m_x = Rect.left;
	m_y = Rect.top;
	m_width = Rect.Width();
	m_height = Rect.Height();

	m_rtPannel = Rect;
}

void CDX3DPannel::GetRect(CRect& Rect)
{
	Rect = m_rtPannel;
}

void CDX3DPannel::SetResourceFont(int nFontID)
{
	m_nFontID = nFontID;
}

void CDX3DPannel::UpdateRects()
{
	m_rtPannel.left		= m_x;
	m_rtPannel.top		= m_y;
	m_rtPannel.right	= m_x + m_width;
	m_rtPannel.bottom	= m_y + m_height;
}

CDX3DResourceManager* CDX3DPannel::GetResource()
{
	return &m_Resource;
}

void CDX3DPannel::SetFocusControl(CDX3DBaseControl* pControl)
{
	if(m_pControlFocus == pControl)
		return;

	m_pControlFocus = pControl;
	if(pControl == NULL)
		return;

	if(!pControl->GetHasFocus())
		return;

	if(m_pControlFocus)
		m_pControlFocus->OnFocusOut();

	pControl->OnFocusIn();
	m_pControlFocus = pControl;
}

void CDX3DPannel::SendEvent(UINT nEvent, CDX3DBaseControl* pControl)
{
	if(m_CallbackEvent)
		m_CallbackEvent(m_nID, nEvent, pControl, m_pUserContext);
}

BOOL CDX3DPannel::MsgProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	if(!m_bVisible)
		return FALSE;

	switch(nMsg)
	{
	case WM_SIZE:
	case WM_MOVE:
		{
			POINT pt = { -1, -1 };
			OnMouseMove(pt);
		}
		break;

	case WM_ACTIVATEAPP:
		if(m_pControlFocus && m_pControlFocus->GetDxPannel() == this && m_pControlFocus->GetEnabled())
		{
			if( wParam )
				m_pControlFocus->OnFocusIn();
			else
				m_pControlFocus->OnFocusOut();
		}
		break;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		{
		}
		break;

	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_XBUTTONDBLCLK:
	case WM_MOUSEWHEEL:
	case WM_MOUSELEAVE:
		{
			//SetFocus(hWnd);

			POINT ptMouse = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			//ptMouse.x -= m_x;
			//ptMouse.y -= m_y;


			if(m_pControlFocus && m_pControlFocus->HandleMouse(nMsg, ptMouse, wParam, lParam))
				return TRUE;


			/*if(m_pControlFocus && m_pControlFocus->GetDxPannel() == this && m_pControlFocus->GetEnabled())
				if(m_pControlFocus->HandleMouse(nMsg, ptMouse, wParam, lParam))
					return TRUE;*/

			CDX3DBaseControl *pControl = GetPtInControl(ptMouse);
			if(pControl && pControl->GetEnabled())
			{
				if(pControl->HandleMouse(nMsg, ptMouse, wParam, lParam))
					return TRUE;
			}
			else
			{
				if(nMsg == WM_LBUTTONDOWN &&
					m_pControlFocus && m_pControlFocus->GetDxPannel() == this)
				{
					m_pControlFocus->OnFocusOut();
					m_pControlFocus = NULL;
				}
			}

			if(nMsg == WM_MOUSEMOVE)
			{
				OnMouseMove(ptMouse, pControl);
				return FALSE;
			}
			else if(nMsg == WM_MOUSELEAVE)
			{
				if(m_pControlMouseOver)
				{
					m_pControlMouseOver->OnMouseLeave();
					m_pControlMouseOver = NULL;
				}
			}
		}
		break;

	case WM_CAPTURECHANGED:
		{
			//if(lParam == m_hParentWnd
		}
		break;
	}

	return FALSE;
}

void CDX3DPannel::OnMouseMove(POINT pt, CDX3DBaseControl* pControl /*= NULL*/)
{
//	TRACE("CDX3DPannel::OnMouseMove x: %4d, y: %4d\n", pt.x, pt.y);
	if(pControl == NULL)
		pControl = GetPtInControl(pt);

	if(m_pControlMouseOver == pControl)
		return;

	if(m_pControlMouseOver)
		m_pControlMouseOver->OnMouseLeave();

	m_pControlMouseOver = pControl;
	if(pControl)
		pControl->OnMouseEnter();
}

void CDX3DPannel::OnMouseUp(POINT pt)
{
	
}

CDX3DBaseControl* CDX3DPannel::GetPtInControl(POINT pt)
{
//	pt.x += m_x;
//	pt.y += m_y;

	std::list<std::tr1::shared_ptr<CDX3DBaseControl>>::iterator pos = m_ControlList.begin();
	while(pos != m_ControlList.end())
	{
		if((*pos)->ContainsPoint(pt) && (*pos)->GetEnabled() && (*pos)->GetVisible())
			return pos->get();

		pos++;
	}

	return NULL;
}

void CDX3DPannel::DrawControl(CDX3DBaseControl* pControl)
{
	if(!pControl->GetVisible())
		return;

	CDX3DResourceManager::DX3D9Texture* pD3dTexture = m_Resource.GetTexture(pControl->GetResourceTexture());

	D3DXVECTOR3 pos((float)pControl->m_x, (float)pControl->m_y, 0.0f);
	
	HRESULT hr = m_Resource.m_pd3dSprite->Draw(pD3dTexture->pTexture,
		&pControl->m_rtTexture, NULL, &pos, D3DCOLOR_ARGB(255,255,255,255));
}

///////////////////////////////////////////////////////////////////////////////////////
// CDX3DBaseControl
CDX3DBaseControl::CDX3DBaseControl(CDX3DPannel* pDxPannel)
: m_pDxPannel(pDxPannel)
, m_nID(SIZE_MAX)
, m_bEnabled(TRUE)
, m_bVisible(TRUE)
, m_bHasFocus(FALSE)
, m_bMouseOver(FALSE)
, m_x(0), m_y(0), m_width(0), m_height(0)
, m_pUserData(NULL)
{
	UpdateRects();
}

CDX3DBaseControl::~CDX3DBaseControl()
{
}

void CDX3DBaseControl::Refresh()
{
	GetDxPannel()->SendEvent(DXGUI_EVENT_REDRAWCONTROL, this);
}

void CDX3DBaseControl::SetLocation(int x, int y)
{
	m_x = x;
	m_y = y;
	
	UpdateRects();
}

void CDX3DBaseControl::SetSize(int width, int height)
{
	m_width = width;
	m_height = height;

	UpdateRects();
}

void CDX3DBaseControl::SetRect(CRect &rect)
{
	m_x = rect.left;
	m_y = rect.top;
	m_width = rect.Width();
	m_height = rect.Height();

	UpdateRects();
}

CRect CDX3DBaseControl::GetRect() const
{
	return m_rcBoundingBox;
}

void CDX3DBaseControl::UpdateRects()
{
	m_rcBoundingBox.left	= m_x;
	m_rcBoundingBox.top		= m_y;
	m_rcBoundingBox.right	= m_x + m_width;
	m_rcBoundingBox.bottom	= m_y + m_height;
}

///////////////////////////////////////////////////////////////////////////////////////