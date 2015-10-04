#pragma once
#pragma comment(lib, "D3D9.lib")
#pragma comment(lib, "D3dx9.lib")
#include <D3D9.h>
#include <D3DX9.h>

#ifdef DISPLAYLIB_EXPORTS
#define DISPLAYLIB_API __declspec(dllexport)
#else
#define DISPLAYLIB_API __declspec(dllimport)
#endif

class DISPLAYLIB_API CD3DRender
{
public:
	CD3DRender(void);
	virtual ~CD3DRender(void);

	BOOL	Initialize(HWND hWnd, int nVideoWidth, int nVideoHeight);
	void	Destroy();
	BOOL	Render(const char *pBuf, size_t nWidth, size_t nHeight);

	void	SetWindowSize(DWORD dwWidth, DWORD dwHeight);
	void	SetEnableAspectratio(BOOL bEnable);
	BOOL	CalWindowToVideo(CRect &rtWindow, CRect *pRect);
	BOOL	CalVideoToWindow(CRect &rtWindow, CRect *pRect);
	void	CalAspectratio();

	BOOL	IsInitialized() const { return m_bIsInitialized; };

protected:
	BOOL	InitVB(float fWidth, float fHeight);
	BOOL	FillPresentationParameters(D3DPRESENT_PARAMETERS *pd3dpp);
	BOOL	ResetDevice();

	BOOL					m_bIsInitialized;
	HWND					m_hWnd;
	SIZE					m_szBackbufferSize;

	LPDIRECT3D9				m_pd3d;
	LPDIRECT3DDEVICE9		m_pd3dDevice;
	LPDIRECT3DVERTEXBUFFER9	m_pVB;

	LPDIRECT3DSURFACE9		m_pd3dSurface;
	SIZE					m_szVideo;			// video size
	CRect					m_rtAspectratio;
	CRect					m_rtWindow;
	LPDIRECT3DTEXTURE9		m_pTexture;

	BOOL					m_bEnableAspectratio;
	
};
