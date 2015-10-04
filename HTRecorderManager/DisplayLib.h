// 다음 ifdef 블록은 DLL에서 내보내기하는 작업을 쉽게 해 주는 매크로를 만드는 
// 표준 방식입니다. 이 DLL에 들어 있는 파일은 모두 명령줄에 정의된 _EXPORTS 기호로
// 컴파일되며, 동일한 DLL을 사용하는 다른 프로젝트에서는 이 기호를 정의할 수 없습니다.
// 이렇게 하면 소스 파일에 이 파일이 들어 있는 다른 모든 프로젝트에서는 
// DISPLAYLIB_API 함수를 DLL에서 가져오는 것으로 보고, 이 DLL은
// 이 DLL은 해당 매크로로 정의된 기호가 내보내지는 것으로 봅니다.
#pragma once
#pragma comment(lib, "D3D9.lib")
#pragma comment(lib, "D3dx9.lib")
#include <D3D9.h>
#include <D3DX9.h>

#include <string>
#include <map>
#include <list>
#include <vector>

//#define _USE_DXDD
#define	_USE_DX3D


#ifndef USE_STATIC_DISPLIB

#ifdef DISPLAYLIB_EXPORTS
#define DISPLAYLIB_API __declspec(dllexport)
#else
#define DISPLAYLIB_API __declspec(dllimport)
#define DEFCALL	   __cdecl
#endif

#else
#define DISPLAYLIB_API
#define DEFCALL
#endif

typedef enum _DISP_VIDEO_OUTPUT
{
	DISP_YV12 = 0,
	DISP_YUY2,
	DISP_RGB24,
	DISP_RGB32,
	DISP_RGB555,
	DISP_RGB565,

} DISP_VIDEO_OUTPUT;

#ifndef _MVMAP
#define _MVMAP
typedef struct _MVINFO {
	RECT rt;
	int xThreshold;
	int yThreshold;
} MVINFO, *PMVINFO;

#endif // _MVMAP

#include <LiveSessionDLL.h>

class CD3DRender;
class CD3DRender2;
class CDDrawRender;
class CAVMedia;
class CLayoutMgr;
class CSpinLock;
class CDisplayLib;
class CWaveAudioSink;

#ifdef __cplusplus
#define EXTERN_C     extern "C"
#else 
#define EXTERN_C
#endif

#ifdef __cplusplus
extern "C" {
#endif

const enum VIEW_LAYOUT
{
	VIEW1x1	= 0,	// 1
	VIEW2x2,		// 4	2x2
	
	VIEW3x3,		// 9	3x3
	VIEW1_5L,		// 6	3x3
	
	VIEW4x4,		// 16	4x4
	VIEW1_7L,		// 8	4x4
	VIEW1_12,		// 13	4x4
	VIEW1_12L,		// 13	4x4
	VIEW2_8,		// 10	4x4

	VIEW5x5,		// 25	5x5
	VIEW1_16,		// 17	5x5
	VIEW1_16L,		// 17	5x5
	VIEW1_9,		// 10	5x5

	VIEW6x6,		// 36
	VIEW1_20,		// 21	6x6
	VIEW1_32,		// 33	6x6
	VIEW1_20L,		// 21	6x6
	VIEW1_27L,		// 28	6x6
	VIEW2_18,		// 20	6x6

	VIEW7x7,		// 49	7x7
	VIEW1_24,		// 25	7x7
	VIEW1_24L,		// 25	7x7
	VIEW1_33L,		// 34	7x7
	VIEW1_40,		// 41	7x7

	VIEW8x8,		// 64	8x8
	VIEW1_28,		// 29	8x8
	VIEW1_28L,		// 29	8x8
	VIEW1_39L,		// 40	8x8
	VIEW1_48,		// 49	8x8
	VIEW1_48L,		// 49	8x8
	VIEW2_32,		// 34	8x8

	// wide
	WVIEW3x2,		// 6	3x2
	WVIEW1_2L,		// 3	3x2

	WVIEW4x3,		// 12	4x3
	WVIEW1_6L,		// 7	4x3
	WVIEW1_3L,		// 4	4x3
	WVIEW2_4,		// 6	4x3

	WVIEW5x4,		// 20	5x4
	WVIEW1_8L,		// 9	5x4
	WVIEW1_4L,		// 5	5x4

	WVIEW6x4,		// 24	6x4
	WVIEW6x5,		// 30	6x5
	WVIEW1_5L,		// 6	6x5
	WVIEW2_12,		// 14	6x5

	MAX_VIEW_LAYOUT
};

#define WM_NOTIFY_DISPLAYLIB	WM_USER + 400

enum NOTIFY_DISPLAYLIB_ID {
	DISP_NOTIFY_RBUTTONUP = 100,
	DISP_NOTIFY_CHANGED_LAYOUT,
	DISP_NOTIFY_SELECTCH,
};

typedef struct _NOTIFY_DISPLAYLIB
{
	HWND		hWnd;		// Handle of Display window
	UINT		Id;			// NOTIFY_DISPLAYLIB_ID value
	UINT		nIndex;		// if -1 is not used, else channel index
	POINT		pt;			// cursor position
	VIEW_LAYOUT Layout;		// current layout

} NOTIFY_DISPLAYLIB;

typedef struct _MEDIA_INFO_T
{
	size_t			nChannel;
	AVMEDIA_TYPE	mediaType;
	BYTE*			pData[4];
	int				nDataSize[4];
	int				nImageWidth;
	int				nImageHeight;
	int				nAudioChannels;
	int				nAudioSampleRate;
	int				nAudioBitPerSample;
	int*			pVideoCSP;
	time_t			time;

} MEDIA_INFO_T, *LPMEDIA_INFO_T;

typedef void (__stdcall *CALLBACK_DISP_RENDER_FUNCTION)(MEDIA_INFO_T *pDispRender, LPVOID pArg);

enum DISPLAY_VIDEOOUTPUT_MODE
{
	DIRECTXDRAW_OVERLAY,
	DIRECTXDRAW_WINDOWED,
	DIRECTXDRAW_RENDERLESS
};

typedef struct _DISP_RECT_INFO
{
	char	RectInfo[64][4];
	int		nMax;
	int		nWidth;
	int		nHeight;

} DISP_RECT_INFO;

#ifdef __cplusplus
}	// extern "C"
#endif

// DirectDraw callbacks
typedef HRESULT (CALLBACK *LPDXDDCALLBACKCUSTOMDRAW)(HDC hdc, DWORD dwIndex, LPRECT lpRect, void* pUserContext);

// Direct3D 9 callbacks
typedef struct _DISP_DX3D9
{
	LPDIRECT3DDEVICE9	pd3dDevice;
	LPD3DXSPRITE		pd3dSprite;
	LPDIRECT3DSURFACE9	pd3dSurface;
	DWORD				dwIndex;
	LPRECT				pRect;
	LPVOID				pUserContext;

} DISP_DX3D9, *LPDISP_DX3D9;

typedef HRESULT	(CALLBACK* LPDXD9CALLBACKDEVICECREATED)(LPDIRECT3DDEVICE9 pd3dDevice, LPD3DXSPRITE pd3dSprite, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
typedef HRESULT	(CALLBACK* LPDXD9CALLBACKDEVICEDESTROYED)(void* pUserContext);
typedef void	(CALLBACK* LPDXD9CALLBACKDEVICERESET)(LPDIRECT3DDEVICE9 pd3dDevice, LPD3DXSPRITE pd3dSprite, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
typedef void	(CALLBACK* LPDXD9CALLBACKCUSTOMDRAW)(LPDISP_DX3D9 pDisp);
typedef void	(CALLBACK* LPDXD9CALLBACKDEVICELOST)(void* pUserContext);
typedef BOOL	(CALLBACK* LPDISPCALLBACKMSGPROC)(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam, void* pUserContext);


#if defined(DISPLAYLIB_EXPORTS) || defined(USE_STATIC_DISPLIB) || defined(DISPLIB_STATIC_LINK)
typedef CDisplayLib* HDISPLIB;
#else
typedef LONG_PTR HDISPLIB;
#endif


class DISPLAYLIB_API CEncoder{
public:
	CEncoder();
	virtual ~CEncoder();
#if 0	// not yet
	BOOL			OpenVideo(AVMEDIA_TYPE Type);
	void			CloseVideo();
	BOOL			EncodeVideo(const char* pSrc, size_t nSrc, char*& pDst, size_t& nDst);
	BOOL			IsOpendVideo();
#endif
	BOOL			OpenAudio(AVMEDIA_TYPE Type, int sample_rate, int bit_rate);
	BOOL			EncodeAudio(const char* pSrc, size_t nSrc, char*& pDst, size_t& nDst);
	void			CloseAudio();
	BOOL			IsOpendAudio();
	AVMEDIA_TYPE	GetAudioType() const;
	size_t			GetAudioBitRate() const;
	size_t			GetAudioSampleRate() const;

protected:
	CAVMedia	*m_pAvMedia;
	CSpinLock	*m_pLock;

	AVMEDIA_TYPE	m_VideoType;
	AVMEDIA_TYPE	m_AudioType;

	size_t			m_AudioBitRate;
	size_t			m_AudioSampleRate;
};

#if defined(DISPLAYLIB_EXPORTS) || defined(USE_STATIC_DISPLIB)
typedef CEncoder* HENCODER;
#else
typedef LONG_PTR HENCODER;

typedef HENCODER	(DEFCALL* LPENCODER_Create)();
typedef void		(DEFCALL* LPENCODER_Release)(HENCODER* pHandle);
typedef BOOL		(DEFCALL* LPENCODER_OpenAudio)(HENCODER handle, AVMEDIA_TYPE Type, int sample_rate, int bit_rate);
typedef BOOL		(DEFCALL* LPENCODER_EncodeAudio)(HENCODER handle, const char* pSrc, size_t nSrc, char*& pDst, size_t& nDst);
typedef void		(DEFCALL* LPENCODER_CloseAudio)(HENCODER handle);
typedef BOOL		(DEFCALL* LPENCODER_IsOpendAudio)(HENCODER handle);
typedef AVMEDIA_TYPE (DEFCALL* LPENCODER_GetAudioType)(HENCODER handle);
typedef size_t		(DEFCALL* LPENCODER_GetAudioBitRate)(HENCODER handle);
typedef size_t		(DEFCALL* LPENCODER_GetAudioSampleRate)(HENCODER handle);

#endif


EXTERN_C DISPLAYLIB_API HENCODER ENCODER_Create();
EXTERN_C DISPLAYLIB_API void ENCODER_Release(HENCODER* ppInstance);
EXTERN_C DISPLAYLIB_API BOOL ENCODER_OpenAudio(HENCODER handle, AVMEDIA_TYPE Type, int sample_rate, int bit_rate);
EXTERN_C DISPLAYLIB_API BOOL ENCODER_EncodeAudio(HENCODER handle, const char* pSrc, size_t nSrc, char*& pDst, size_t& nDst);
EXTERN_C DISPLAYLIB_API void ENCODER_CloseAudio(HENCODER handle);
EXTERN_C DISPLAYLIB_API BOOL ENCODER_IsOpendAudio(HENCODER handle);
EXTERN_C DISPLAYLIB_API AVMEDIA_TYPE ENCODER_GetAudioType(HENCODER handle);
EXTERN_C DISPLAYLIB_API size_t ENCODER_GetAudioBitRate(HENCODER handle);
EXTERN_C DISPLAYLIB_API size_t ENCODER_GetAudioSampleRate(HENCODER handle);


typedef struct _RENDER_INFO
{
	HDISPLIB	pDisp;
	MEDIA_INFO_T	MediaInfo;

} RENDER_INFO_T, *LPRENDER_INFO_T;

typedef enum _IMAGE_TYPE
{
	IMAGE_YUY2	= (1<<3),
	IMAGE_BGRA	= (1<<6),
	IMAGE_BGR	= (1<<9),
	IMAGE_RGB555	= (1<<10),
	IMAGE_RGB565	= (1<<11),
	IMAGE_VFLIP		= (1<<31),
} IMAGE_TYPE;


class DISPLAYLIB_API CDecoder{
public:
	CDecoder();
	virtual ~CDecoder(void);

	static CDecoder*	CreateDecoder();
	static void			DestroyDecoder(CDecoder* p);
	static void			FreeMV(PMVINFO pmvinfo);

	BOOL	Process(GRSTREAM_BUFFER *pBuffer, UINT nFilter = 0);
	BOOL	Decode(GRSTREAM_BUFFER *pBuffer, MEDIA_INFO_T *pMediaInfo, UINT nFilter = 0);
	void	Refresh(const char* pKey);
	BOOL	IsRemainVideoBuffer();
	void	Restore();

	BOOL	IsOpenVideo();
	BOOL	IsOpenAudio();

	void	Close();

	BOOL	AddCallbackFunction(const char *pKey, HDISPLIB pDisp, size_t nChannel);
	void	UpdateCallbackFunction(const char *pKey, HDISPLIB pDisp, size_t nChannel);
	size_t	RemoveCallbackFunction(const char *pKey, HDISPLIB pDisp, size_t nChannel, BOOL bAutoClose = FALSE);
	void	RemoveCallbackFunctionAll(BOOL bAutoClose = FALSE);
	size_t	GetDisplayCount(const char *pKey);

	BYTE*	GetSnapshot(size_t& nBufferSize, SIZE& nImageSize, IMAGE_TYPE Type = IMAGE_YUY2);

	BOOL	IsDecodeAudio() const;
	void	SetDecodeAudio(BOOL bEnable);

	size_t	GetVideoDecodeCount() const;

	void	Lock();
	void	Unlock();

	void	SetVideoOutput(DISP_VIDEO_OUTPUT VideoOutput);

	// Smart Search
	bool m_bSSMode;
	int   m_nSSChannel;
	void  SetSmtSearchModeCh(bool bMode, int nSelCh);
	bool  GetSmtSearchMode() const { return m_bSSMode; }
	int    GetSmtSearchChannel() const { return m_nSSChannel; }
			
	BOOL	GetMVInfo(PMVINFO &pmvinfo, int *pnSize);
	void	ClearMVInfo();
		
	void	SetEnableDecodeMode(BOOL bEnable);
	BOOL	IsEnabledDecodeMode() const;
	BOOL	SetDecodeMode(BOOL bDecode);
	BOOL	GetDecodeMode() const;
		
protected:
	CAVMedia	*m_pAvMedia;
	BYTE		*m_pSnapshotBuffer;
	std::multimap<std::string, RENDER_INFO_T> *m_mapCallback;
	CRITICAL_SECTION m_cs;

	time_t		m_tVideoDecode;
	size_t		m_nVideoDecode;
	size_t		m_nVideoDecodeCount;

	int			m_nVideoType;
	int			m_nAudioType;

	std::list<MVINFO>*	m_pMVList;
	MVINFO*				m_pMVInfo;
	size_t				m_nMVSize;
	inline BOOL _Decode(CAVMedia *pMedia, GRSTREAM_BUFFER *pBuffer, BOOL bKeyFrame, BOOL bRetryEncode = FALSE, std::list<MVINFO>* mvList = NULL);

	BOOL		m_bUseDecodeMode;
	BOOL		m_bDecodeMode;
	size_t		m_nModeCount;
	time_t		m_nModeTime;
};


#if defined(DISPLAYLIB_EXPORTS) || defined(USE_STATIC_DISPLIB)
typedef CDecoder* HDECODER;
#else
typedef LONG_PTR HDECODER;

typedef HDECODER	(DEFCALL* LPDECODER_Create)();
typedef void		(DEFCALL* LPDECODER_Release)(HDECODER* pHandle);
typedef BOOL		(DEFCALL* LPDECODER_Process)(HDECODER handle, GRSTREAM_BUFFER *pBuffer, UINT nFilter);
typedef BOOL		(DEFCALL* LPDECODER_Decode)(HDECODER handle, GRSTREAM_BUFFER *pBuffer, MEDIA_INFO_T *pMediaInfo, UINT nFilter);
typedef void		(DEFCALL* LPDECODER_Refresh)(HDECODER handle, const char* pKey);
typedef BOOL		(DEFCALL* LPDECODER_IsRemainVideoBuffer)(HDECODER handle);
typedef void		(DEFCALL* LPDECODER_Restore)(HDECODER handle);
typedef BOOL		(DEFCALL* LPDECODER_IsOpenVideo)(HDECODER handle);
typedef BOOL		(DEFCALL* LPDECODER_IsOpenAudio)(HDECODER handle);
typedef void		(DEFCALL* LPDECODER_Close)(HDECODER handle);
typedef BOOL		(DEFCALL* LPDECODER_AddCallbackFunction)(HDECODER handle, const char *pKey, HDISPLIB pDisp, size_t nChannel);
typedef void		(DEFCALL* LPDECODER_UpdateCallbackFunction)(HDECODER handle, const char *pKey, HDISPLIB pDisp, size_t nChannel);
typedef size_t		(DEFCALL* LPDECODER_RemoveCallbackFunction)(HDECODER handle, const char *pKey, HDISPLIB pDisp, size_t nChannel, BOOL bAutoClose);
typedef void		(DEFCALL* LPDECODER_RemoveCallbackFunctionAll)(HDECODER handle, BOOL bAutoClose);
typedef size_t		(DEFCALL* LPDECODER_GetDisplayCount)(HDECODER handle, const char *pKey);
typedef BYTE*		(DEFCALL* LPDECODER_GetSnapshot)(HDECODER handle, size_t& nBufferSize, SIZE& nImageSize, IMAGE_TYPE Type);
typedef BOOL		(DEFCALL* LPDECODER_IsDecodeAudio)(HDECODER handle);
typedef void		(DEFCALL* LPDECODER_SetDecodeAudio)(HDECODER handle, BOOL bEnable);
typedef size_t		(DEFCALL* LPDECODER_GetVideoDecodeCount)(HDECODER handle);
typedef void		(DEFCALL* LPDECODER_Lock)(HDECODER handle);
typedef void		(DEFCALL* LPDECODER_Unlock)(HDECODER handle);
typedef void		(DEFCALL* LPDECODER_SetVideoOutput)(HDECODER handle, DISP_VIDEO_OUTPUT VideoOutput);
typedef void		(DEFCALL* LPDECODER_SetSmtSearchModeCh)(HDECODER handle, bool bMode, int nSelCh);
typedef bool		(DEFCALL* LPDECODER_GetSmtSearchMode)(HDECODER handle);
typedef int			(DEFCALL* LPDECODER_GetSmtSearchChannel)(HDECODER handle);
typedef BOOL		(DEFCALL* LPDECODER_GetMVInfo)(HDECODER handle, PMVINFO &pmvinfo, int *pnSize);
typedef void		(DEFCALL* LPDECODER_ClearMVInfo)(HDECODER handle);
typedef void		(DEFCALL* LPDECODER_SetEnableDecodeMode)(HDECODER handle, BOOL bEnable);
typedef BOOL		(DEFCALL* LPDECODER_IsEnabledDecodeMode)(HDECODER handle);
typedef BOOL		(DEFCALL* LPDECODER_SetDecodeMode)(HDECODER handle, BOOL bDecode);
typedef BOOL		(DEFCALL* LPDECODER_GetDecodeMode)(HDECODER handle);

#endif

EXTERN_C DISPLAYLIB_API HDECODER DECODER_Create();
EXTERN_C DISPLAYLIB_API void DECODER_Release(HDECODER* pHandle);
EXTERN_C DISPLAYLIB_API BOOL DECODER_Process(HDECODER handle, GRSTREAM_BUFFER *pBuffer, UINT nFilter);
EXTERN_C DISPLAYLIB_API BOOL DECODER_Decode(HDECODER handle, GRSTREAM_BUFFER *pBuffer, MEDIA_INFO_T *pMediaInfo, UINT nFilter);
EXTERN_C DISPLAYLIB_API void DECODER_Refresh(HDECODER handle, const char* pKey);
EXTERN_C DISPLAYLIB_API BOOL DECODER_IsRemainVideoBuffer(HDECODER handle);
EXTERN_C DISPLAYLIB_API void DECODER_Restore(HDECODER handle);
EXTERN_C DISPLAYLIB_API BOOL DECODER_IsOpenVideo(HDECODER handle);
EXTERN_C DISPLAYLIB_API BOOL DECODER_IsOpenAudio(HDECODER handle);
EXTERN_C DISPLAYLIB_API void DECODER_Close(HDECODER handle);
EXTERN_C DISPLAYLIB_API BOOL DECODER_AddCallbackFunction(HDECODER handle, const char *pKey, HDISPLIB pDisp, size_t nChannel);
EXTERN_C DISPLAYLIB_API void DECODER_UpdateCallbackFunction(HDECODER handle, const char *pKey, HDISPLIB pDisp, size_t nChannel);
EXTERN_C DISPLAYLIB_API size_t DECODER_RemoveCallbackFunction(HDECODER handle, const char *pKey, HDISPLIB pDisp, size_t nChannel, BOOL bAutoClose);
EXTERN_C DISPLAYLIB_API void DECODER_RemoveCallbackFunctionAll(HDECODER handle, BOOL bAutoClose);
EXTERN_C DISPLAYLIB_API size_t DECODER_GetDisplayCount(HDECODER handle, const char *pKey);
EXTERN_C DISPLAYLIB_API BYTE* DECODER_GetSnapshot(HDECODER handle, size_t& nBufferSize, SIZE& nImageSize, IMAGE_TYPE Type);
EXTERN_C DISPLAYLIB_API BOOL DECODER_IsDecodeAudio(HDECODER handle);
EXTERN_C DISPLAYLIB_API void DECODER_SetDecodeAudio(HDECODER handle, BOOL bEnable);
EXTERN_C DISPLAYLIB_API size_t DECODER_GetVideoDecodeCount(HDECODER handle);
EXTERN_C DISPLAYLIB_API void DECODER_Lock(HDECODER handle);
EXTERN_C DISPLAYLIB_API void DECODER_Unlock(HDECODER handle);
EXTERN_C DISPLAYLIB_API void DECODER_SetVideoOutput(HDECODER handle, DISP_VIDEO_OUTPUT VideoOutput);
EXTERN_C DISPLAYLIB_API void DECODER_SetSmtSearchModeCh(HDECODER handle, bool bMode, int nSelCh);
EXTERN_C DISPLAYLIB_API bool DECODER_GetSmtSearchMode(HDECODER handle);
EXTERN_C DISPLAYLIB_API int DECODER_GetSmtSearchChannel(HDECODER handle);
EXTERN_C DISPLAYLIB_API BOOL DECODER_GetMVInfo(HDECODER handle, PMVINFO &pmvinfo, int *pnSize);
EXTERN_C DISPLAYLIB_API void DECODER_ClearMVInfo(HDECODER handle);
EXTERN_C DISPLAYLIB_API void DECODER_SetEnableDecodeMode(HDECODER handle, BOOL bEnable);
EXTERN_C DISPLAYLIB_API BOOL DECODER_IsEnabledDecodeMode(HDECODER handle);
EXTERN_C DISPLAYLIB_API BOOL DECODER_SetDecodeMode(HDECODER handle, BOOL bDecode);
EXTERN_C DISPLAYLIB_API BOOL DECODER_GetDecodeMode(HDECODER handle);

class DISPLAYLIB_API CDisplayLib{
public:
	CDisplayLib(void);
	virtual ~CDisplayLib(void);

	static CDisplayLib*	CreateDisplayLib();
	static void DestroyDisplayLib(CDisplayLib* pLib);

	void MediaRender(MEDIA_INFO_T *pMediaInfo);

	static BOOL GetLayoutRectInfo(VIEW_LAYOUT viewLayout, DISP_RECT_INFO* pDispRectInfo);
	static int GetLayoutViewSize(VIEW_LAYOUT viewLayout);

	BOOL		Create(DWORD dwMaxChannel, DWORD dwStyle, RECT rect, HINSTANCE hInstance, HWND hWndParent, DISPLAY_VIDEOOUTPUT_MODE VideoOutputMode = DIRECTXDRAW_RENDERLESS);
	void		Destroy();
	void		SetWindowSize(int nWidth, int nHeight);

	int			GetCurrentSelectedIndex() const { return GetSelectedIndex(); }

	void		SetFreeze(int nIndex, BOOL bEnable);
	BOOL		IsFrozen(int nIndex) const;

	void		SetEnableAspectratio(int nIndex, BOOL bEnable);
	BOOL		IsEnableAspectratio(int nIndex) const;

	BOOL		HaveDrawnImage(int nIndex);
	void		ClearVideo(int nIndex);
	void		ClearVideoAll();
	void		SetPreview(DWORD dwIndex, LPCTSTR lpszText = NULL);

	void		SetViewLayout(VIEW_LAYOUT viewLayout);
	VIEW_LAYOUT GetCurrentViewLayout() const;
	VIEW_LAYOUT GetViewLayout() const;

	int			GetIndex(POINT pt);
	int			SetSelectIndex(POINT pt);
	int			SetSelectIndex(DWORD dwIndex);
	int			GetSelectedIndex() const;
	DWORD		GetMaxChannel() const;
	int			GetCurrentViewSize();
	int			GoNextSplitGroup();
	int			GoPreviousSplitGroup();

	BOOL		GetVideoSize(DWORD dwIndex, LPSIZE pSize);

	void		Invalidate(DWORD dwIndex);
	void		HideChannel(DWORD dwIndex, BOOL bHide);

	////////////////////////////////////////////////////////
	// Color 지정
	void		SetBackgroundColor(COLORREF color = RGB(0, 0, 0));
	void		SetBackgroundNoSelectColor(COLORREF color = RGB(10, 10, 10));
	COLORREF	GetBackgroundColor() const;
	COLORREF	GetBackgroundNoSelectColor() const;
	void		SetOverlayColorKey(COLORREF color);
	COLORREF	GetOverlayColorKey() const;

	////////////////////////////////////////////////////////
	// Digital PTZ 관련
	BOOL		GetRect(DWORD dwIndex, LPRECT pRect);		// aspectratio가 적용되어있다면 해당 사이즈 (비디오가 보여지는) 가 리턴됨
	BOOL		GetRect2(DWORD dwIndex, LPRECT pRect);		// 채널의 윈도우 크기
	void		SetDrawIndicationRect(DWORD dwIndex, BOOL bDraw, LPRECT pRect = NULL);
	void		SetDZoomMode(DWORD dwIndex, BOOL bEnable, LPRECT pRect = NULL);
	BOOL		IsDZoomMode(DWORD dwIndex) const;
	BOOL		DZoomIndicationPtInRect(DWORD dwIndex, POINT pt);
	BOOL		DZoomPicturePtInRect(DWORD dwIndex, POINT pt);
	void		OffsetDZoom(DWORD dwIndex, int x, int y);
	void		InflateDZoom(DWORD dwIndex, int x, int y);
	void		SetEnableDZoom(DWORD dwIndex, BOOL bEnable);
	BOOL		IsEnableDZoom(DWORD dwIndex) const;

	////////////////////////////////////////////////////////
	// Snapshot (OUTPUT Bitmap 이미지 파일)
	BOOL		GetBitmapImage(LPRECT pSrc, BYTE** ppOut, UINT* pOutSize);
	BOOL		GetBitmapImage(DWORD dwIndex, BYTE** ppOut, UINT* pOutSize);

	////////////////////////////////////////////////////////
	// 오디오 Sound On/Off
	void		SetSoundOn(DWORD dwIndex, BOOL bOn = TRUE);
	BOOL		IsSoundOn(DWORD dwIndex) const;

	HWND		m_hWnd;

	void		SetUserContext(void* pUserContext);
#if defined(_USE_DXDD)
	void		SetCallbackDDCustomDraw(LPDXDDCALLBACKCUSTOMDRAW lpDDCustomDraw);
#else
	void		SetCallbackD3D9DeviceCreated(LPDXD9CALLBACKDEVICECREATED pd3dDeviceCreated);
	void		SetCallbackD3D9DeviceDestroyed(LPDXD9CALLBACKDEVICEDESTROYED pd3dDeviceDestroyed);
	void		SetCallbackD3D9DeviceLost(LPDXD9CALLBACKDEVICELOST pd3dDeviceLost);
	void		SetCallbackD3D9DeviceReset(LPDXD9CALLBACKDEVICERESET pd3dDeviceReset);
	void		SetCallbackD3D9CustomDraw(LPDXD9CALLBACKCUSTOMDRAW pd3dCustomDraw);
	void		SetCallbackDispMsgProc(LPDISPCALLBACKMSGPROC pd3dMsgProc);
#endif

	void		UpdateTime(DWORD dwIndex, time_t time);
	time_t		GetUpdatedTime(DWORD dwIndex) const;

	void		SetEnableChannelBackground(DWORD dwIndex, BOOL bEnable);
	BOOL		IsEnableChannelBackground(DWORD dwIndex) const;
	void		SetChannelBackground(DWORD dwIndex, D3DCOLOR color);
	D3DCOLOR	GetChannelBackground(DWORD dwIndex) const;

	HWND		GetHwnd() { return m_hWnd; };

	void		SetEnableBackgroundImage(BOOL bEnable);
	BOOL		IsEnabledBackgroundImage() const;
	BOOL		SetBackgroundImage(LPCTSTR lpFilePath, D3DCOLOR colorKey);
	BOOL		SetBackgroundImage(void* pStream, UINT nLength, D3DCOLOR colorKey);

protected:
	class CDispInfo
	{
	public:
		CDispInfo();
		~CDispInfo();

	protected:
		// 비디오 관련
		BOOL			bFreeze;

		// 오디오 관련
		BOOL			bSoundOn;
		CWaveAudioSink*	pAudioSink;

		friend class CDisplayLib;
	};

	HINSTANCE	m_hInst;
	DWORD		m_dwMaxChannel;

	static ATOM m_sClassAtom;
	static TCHAR m_szClassName[];

	static LRESULT __stdcall WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	ATOM GetAtomClass(HINSTANCE hInstance);

	LRESULT OnCreate(HWND hWnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnLButtonUp(HWND hWnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnLButtonDblClk(HWND hWnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnMouseWheel(HWND hWnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnRButtonUp(HWND hWnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam);

	inline void ProcHitronEventStream(CAVMedia *pMedia, GRSTREAM_BUFFER *pBuffer);

	///////////////////////////////////////////////////////////////////////////////////////////
#if defined(_USE_DXDD)
	CDDrawRender	*m_pDisplay;
	LPDXDDCALLBACKCUSTOMDRAW		m_pddCustomDraw;
#else
	CD3DRender2		*m_pDisplay;
	LPDXD9CALLBACKDEVICECREATED		m_pd3dDeviceCreated;
	LPDXD9CALLBACKDEVICEDESTROYED	m_pd3dDeviceDestroyed;
	LPDXD9CALLBACKDEVICELOST		m_pd3dDeviceLost;
	LPDXD9CALLBACKDEVICERESET		m_pd3dDeviceReset;
	LPDXD9CALLBACKCUSTOMDRAW		m_pd3dCustomDraw;
	LPDISPCALLBACKMSGPROC			m_pd3dMsgProc;
#endif
	void*		m_pUserContext;
	SIZE		m_szWindow;

	DISPLAY_VIDEOOUTPUT_MODE	m_VideoOutputMode;
	CDispInfo		*m_pDispInfo;

	time_t		m_tOldTime;
	time_t		m_tPlayTime;
	tm			m_tmPlay;

	BOOL		m_bMouseDown;
	POINT		m_ptStartIndication;
	RECT		m_rtCurSelect;
	RECT		m_rtIndication;
};


#ifndef DISPLAYLIB_EXPORTS

typedef HDISPLIB	(DEFCALL* LPDISPLIB_Create)();
typedef void		(DEFCALL* LPDISPLIB_Release)(HDISPLIB* pHandle);
typedef void		(DEFCALL* LPDISPLIB_MediaRender)(HDISPLIB handle, MEDIA_INFO_T *pMediaInfo);
typedef BOOL		(DEFCALL* LPDISPLIB_CreateWindow)(HDISPLIB handle, DWORD dwMaxChannel, DWORD dwStyle, RECT rect, HINSTANCE hInstance, HWND hWndParent, DISPLAY_VIDEOOUTPUT_MODE VideoOutputMode);
typedef void		(DEFCALL* LPDISPLIB_DestroyWindow)(HDISPLIB handle);
typedef void		(DEFCALL* LPDISPLIB_SetWindowSize)(HDISPLIB handle, int nWidth, int nHeight);
typedef int			(DEFCALL* LPDISPLIB_GetCurrentSelectedIndex)(HDISPLIB handle);
typedef void		(DEFCALL* LPDISPLIB_SetFreeze)(HDISPLIB handle, int nIndex, BOOL bEnable);
typedef BOOL		(DEFCALL* LPDISPLIB_IsFrozen)(HDISPLIB handle, int nIndex);
typedef void		(DEFCALL* LPDISPLIB_SetEnableAspectratio)(HDISPLIB handle, int nIndex, BOOL bEnable);
typedef BOOL		(DEFCALL* LPDISPLIB_IsEnableAspectratio)(HDISPLIB handle, int nIndex);
typedef BOOL		(DEFCALL* LPDISPLIB_HaveDrawnImage)(HDISPLIB handle, int nIndex);
typedef void		(DEFCALL* LPDISPLIB_ClearVideo)(HDISPLIB handle, int nIndex);
typedef void		(DEFCALL* LPDISPLIB_ClearVideoAll)(HDISPLIB handle);
typedef void		(DEFCALL* LPDISPLIB_SetPreview)(HDISPLIB handle, DWORD dwIndex, LPCTSTR lpszText);
typedef void		(DEFCALL* LPDISPLIB_SetViewLayout)(HDISPLIB handle, VIEW_LAYOUT viewLayout);
typedef VIEW_LAYOUT	(DEFCALL* LPDISPLIB_GetCurrentViewLayout)(HDISPLIB handle);
typedef VIEW_LAYOUT (DEFCALL* LPDISPLIB_GetViewLayout)(HDISPLIB handle);
typedef int			(DEFCALL* LPDISPLIB_GetIndex)(HDISPLIB handle, POINT pt);
typedef int			(DEFCALL* LPDISPLIB_SetSelectIndexByPt)(HDISPLIB handle, POINT pt);
typedef int			(DEFCALL* LPDISPLIB_SetSelectIndexByIdx)(HDISPLIB handle, DWORD dwIndex);
typedef int			(DEFCALL* LPDISPLIB_GetSelectedIndex)(HDISPLIB handle);
typedef DWORD		(DEFCALL* LPDISPLIB_GetMaxChannel)(HDISPLIB handle);
typedef int			(DEFCALL* LPDISPLIB_GetCurrentViewSize)(HDISPLIB handle);
typedef int			(DEFCALL* LPDISPLIB_GoNextSplitGroup)(HDISPLIB handle);
typedef int			(DEFCALL* LPDISPLIB_GoPreviousSplitGroup)(HDISPLIB handle);
typedef BOOL		(DEFCALL* LPDISPLIB_GetVideoSize)(HDISPLIB handle, DWORD dwIndex, LPSIZE pSize);
typedef void		(DEFCALL* LPDISPLIB_Invalidate)(HDISPLIB handle, DWORD dwIndex);
typedef void		(DEFCALL* LPDISPLIB_HideChannel)(HDISPLIB handle, DWORD dwIndex, BOOL bHide);
typedef void		(DEFCALL* LPDISPLIB_SetBackgroundColor)(HDISPLIB handle, COLORREF color);
typedef void		(DEFCALL* LPDISPLIB_SetBackgroundNoSelectColor)(HDISPLIB handle, COLORREF color);
typedef COLORREF	(DEFCALL* LPDISPLIB_GetBackgroundColor)(HDISPLIB handle);
typedef COLORREF	(DEFCALL* LPDISPLIB_GetBackgroundNoSelectColor)(HDISPLIB handle);
typedef void		(DEFCALL* LPDISPLIB_SetOverlayColorKey)(HDISPLIB handle, COLORREF color);
typedef COLORREF	(DEFCALL* LPDISPLIB_GetOverlayColorKey)(HDISPLIB handle);
typedef BOOL		(DEFCALL* LPDISPLIB_GetRect)(HDISPLIB handle, DWORD dwIndex, LPRECT pRect);
typedef BOOL		(DEFCALL* LPDISPLIB_GetRect2)(HDISPLIB handle, DWORD dwIndex, LPRECT pRect);
typedef void		(DEFCALL* LPDISPLIB_SetDrawIndicationRect)(HDISPLIB handle, DWORD dwIndex, BOOL bDraw, LPRECT pRect);
typedef void		(DEFCALL* LPDISPLIB_SetDZoomMode)(HDISPLIB handle, DWORD dwIndex, BOOL bEnable, LPRECT pRect);
typedef BOOL		(DEFCALL* LPDISPLIB_IsDZoomMode)(HDISPLIB handle, DWORD dwIndex);
typedef BOOL		(DEFCALL* LPDISPLIB_DZoomIndicationPtInRect)(HDISPLIB handle, DWORD dwIndex, POINT pt);
typedef BOOL		(DEFCALL* LPDISPLIB_DZoomPicturePtInRect)(HDISPLIB handle, DWORD dwIndex, POINT pt);
typedef void		(DEFCALL* LPDISPLIB_OffsetDZoom)(HDISPLIB handle, DWORD dwIndex, int x, int y);
typedef void		(DEFCALL* LPDISPLIB_InflateDZoom)(HDISPLIB handle, DWORD dwIndex, int x, int y);
typedef void		(DEFCALL* LPDISPLIB_SetEnableDZoom)(HDISPLIB handle, DWORD dwIndex, BOOL bEnable);
typedef BOOL		(DEFCALL* LPDISPLIB_IsEnableDZoom)(HDISPLIB handle, DWORD dwIndex);
typedef BOOL		(DEFCALL* LPDISPLIB_GetBitmapImageByRect)(HDISPLIB handle, LPRECT pSrc, BYTE** ppOut, UINT* pOutSize);
typedef BOOL		(DEFCALL* LPDISPLIB_GetBitmapImageByIdx)(HDISPLIB handle, DWORD dwIndex, BYTE** ppOut, UINT* pOutSize);
typedef void		(DEFCALL* LPDISPLIB_SetSoundOn)(HDISPLIB handle, DWORD dwIndex, BOOL bOn);
typedef BOOL		(DEFCALL* LPDISPLIB_IsSoundOn)(HDISPLIB handle, DWORD dwIndex);
typedef void		(DEFCALL* LPDISPLIB_SetUserContext)(HDISPLIB handle, void* pUserContext);
#if defined(_USE_DXDD)
typedef void		(DEFCALL* LPDISPLIB_SetCallbackDDCustomDraw)(HDISPLIB handle, LPDXDDCALLBACKCUSTOMDRAW lpDDCustomDraw);
#else
typedef void		(DEFCALL* LPDISPLIB_SetCallbackD3D9DeviceCreated)(HDISPLIB handle, LPDXD9CALLBACKDEVICECREATED pd3dDeviceCreated);
typedef void		(DEFCALL* LPDISPLIB_SetCallbackD3D9DeviceDestroyed)(HDISPLIB handle, LPDXD9CALLBACKDEVICEDESTROYED pd3dDeviceDestroyed);
typedef void		(DEFCALL* LPDISPLIB_SetCallbackD3D9DeviceLost)(HDISPLIB handle, LPDXD9CALLBACKDEVICELOST pd3dDeviceLost);
typedef void		(DEFCALL* LPDISPLIB_SetCallbackD3D9DeviceReset)(HDISPLIB handle, LPDXD9CALLBACKDEVICERESET pd3dDeviceReset);
typedef void		(DEFCALL* LPDISPLIB_SetCallbackD3D9CustomDraw)(HDISPLIB handle, LPDXD9CALLBACKCUSTOMDRAW pd3dCustomDraw);
typedef void		(DEFCALL* LPDISPLIB_SetCallbackDispMsgProc)(HDISPLIB handle, LPDISPCALLBACKMSGPROC pd3dMsgProc);
#endif
typedef void		(DEFCALL* LPDISPLIB_UpdateTime)(HDISPLIB handle, DWORD dwIndex, time_t time);
typedef time_t		(DEFCALL* LPDISPLIB_GetUpdatedTime)(HDISPLIB handle, DWORD dwIndex);
typedef void		(DEFCALL* LPDISPLIB_SetEnableChannelBackground)(HDISPLIB handle, DWORD dwIndex, BOOL bEnable);
typedef BOOL		(DEFCALL* LPDISPLIB_IsEnableChannelBackground)(HDISPLIB handle, DWORD dwIndex);
typedef void		(DEFCALL* LPDISPLIB_SetChannelBackground)(HDISPLIB handle, DWORD dwIndex, D3DCOLOR color);
typedef D3DCOLOR	(DEFCALL* LPDISPLIB_GetChannelBackground)(HDISPLIB handle, DWORD dwIndex);
typedef HWND		(DEFCALL* LPDISPLIB_GetHwnd)(HDISPLIB handle);


#endif

EXTERN_C DISPLAYLIB_API HDISPLIB DISPLIB_Create();
EXTERN_C DISPLAYLIB_API void DISPLIB_Release(HDISPLIB* ppInstance);
EXTERN_C DISPLAYLIB_API void DISPLIB_MediaRender(HDISPLIB handle, MEDIA_INFO_T *pMediaInfo);
EXTERN_C DISPLAYLIB_API	BOOL DISPLIB_CreateWindow(HDISPLIB handle, DWORD dwMaxChannel, DWORD dwStyle, RECT rect, HINSTANCE hInstance, HWND hWndParent, DISPLAY_VIDEOOUTPUT_MODE VideoOutputMode);
EXTERN_C DISPLAYLIB_API	void DISPLIB_DestroyWindow(HDISPLIB handle);
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetWindowSize(HDISPLIB handle, int nWidth, int nHeight);
EXTERN_C DISPLAYLIB_API	int	DISPLIB_GetCurrentSelectedIndex(HDISPLIB handle);
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetFreeze(HDISPLIB handle, int nIndex, BOOL bEnable);
EXTERN_C DISPLAYLIB_API	BOOL DISPLIB_IsFrozen(HDISPLIB handle, int nIndex);
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetEnableAspectratio(HDISPLIB handle, int nIndex, BOOL bEnable);
EXTERN_C DISPLAYLIB_API	BOOL DISPLIB_IsEnableAspectratio(HDISPLIB handle, int nIndex);
EXTERN_C DISPLAYLIB_API	BOOL DISPLIB_HaveDrawnImage(HDISPLIB handle, int nIndex);
EXTERN_C DISPLAYLIB_API	void DISPLIB_ClearVideo(HDISPLIB handle, int nIndex);
EXTERN_C DISPLAYLIB_API	void DISPLIB_ClearVideoAll(HDISPLIB handle);
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetPreview(HDISPLIB handle, DWORD dwIndex, LPCTSTR lpszText);
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetViewLayout(HDISPLIB handle, VIEW_LAYOUT viewLayout);
EXTERN_C DISPLAYLIB_API	VIEW_LAYOUT DISPLIB_GetCurrentViewLayout(HDISPLIB handle);
EXTERN_C DISPLAYLIB_API	VIEW_LAYOUT DISPLIB_GetViewLayout(HDISPLIB handle);
EXTERN_C DISPLAYLIB_API	int	DISPLIB_GetIndex(HDISPLIB handle, POINT pt);
EXTERN_C DISPLAYLIB_API	int	DISPLIB_SetSelectIndexByPt(HDISPLIB handle, POINT pt);
EXTERN_C DISPLAYLIB_API	int	DISPLIB_SetSelectIndexByIdx(HDISPLIB handle, DWORD dwIndex);
EXTERN_C DISPLAYLIB_API	int	DISPLIB_GetSelectedIndex(HDISPLIB handle);
EXTERN_C DISPLAYLIB_API	DWORD DISPLIB_GetMaxChannel(HDISPLIB handle);
EXTERN_C DISPLAYLIB_API	int	DISPLIB_GetCurrentViewSize(HDISPLIB handle);
EXTERN_C DISPLAYLIB_API	int	DISPLIB_GoNextSplitGroup(HDISPLIB handle);
EXTERN_C DISPLAYLIB_API	int	DISPLIB_GoPreviousSplitGroup(HDISPLIB handle);
EXTERN_C DISPLAYLIB_API	BOOL DISPLIB_GetVideoSize(HDISPLIB handle, DWORD dwIndex, LPSIZE pSize);
EXTERN_C DISPLAYLIB_API	void DISPLIB_Invalidate(HDISPLIB handle, DWORD dwIndex);
EXTERN_C DISPLAYLIB_API	void DISPLIB_HideChannel(HDISPLIB handle, DWORD dwIndex, BOOL bHide);
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetBackgroundColor(HDISPLIB handle, COLORREF color);
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetBackgroundNoSelectColor(HDISPLIB handle, COLORREF color);
EXTERN_C DISPLAYLIB_API	COLORREF DISPLIB_GetBackgroundColor(HDISPLIB handle);
EXTERN_C DISPLAYLIB_API	COLORREF DISPLIB_GetBackgroundNoSelectColor(HDISPLIB handle);
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetOverlayColorKey(HDISPLIB handle, COLORREF color);
EXTERN_C DISPLAYLIB_API	COLORREF DISPLIB_GetOverlayColorKey(HDISPLIB handle);
EXTERN_C DISPLAYLIB_API	BOOL DISPLIB_GetRect(HDISPLIB handle, DWORD dwIndex, LPRECT pRect);
EXTERN_C DISPLAYLIB_API	BOOL DISPLIB_GetRect2(HDISPLIB handle, DWORD dwIndex, LPRECT pRect);
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetDrawIndicationRect(HDISPLIB handle, DWORD dwIndex, BOOL bDraw, LPRECT pRect);
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetDZoomMode(HDISPLIB handle, DWORD dwIndex, BOOL bEnable, LPRECT pRect);
EXTERN_C DISPLAYLIB_API	BOOL DISPLIB_IsDZoomMode(HDISPLIB handle, DWORD dwIndex);
EXTERN_C DISPLAYLIB_API	BOOL DISPLIB_DZoomIndicationPtInRect(HDISPLIB handle, DWORD dwIndex, POINT pt);
EXTERN_C DISPLAYLIB_API	BOOL DISPLIB_DZoomPicturePtInRect(HDISPLIB handle, DWORD dwIndex, POINT pt);
EXTERN_C DISPLAYLIB_API	void DISPLIB_OffsetDZoom(HDISPLIB handle, DWORD dwIndex, int x, int y);
EXTERN_C DISPLAYLIB_API	void DISPLIB_InflateDZoom(HDISPLIB handle, DWORD dwIndex, int x, int y);
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetEnableDZoom(HDISPLIB handle, DWORD dwIndex, BOOL bEnable);
EXTERN_C DISPLAYLIB_API	BOOL DISPLIB_IsEnableDZoom(HDISPLIB handle, DWORD dwIndex);
EXTERN_C DISPLAYLIB_API	BOOL DISPLIB_GetBitmapImageByRect(HDISPLIB handle, LPRECT pSrc, BYTE** ppOut, UINT* pOutSize);
EXTERN_C DISPLAYLIB_API	BOOL DISPLIB_GetBitmapImageByIdx(HDISPLIB handle, DWORD dwIndex, BYTE** ppOut, UINT* pOutSize);
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetSoundOn(HDISPLIB handle, DWORD dwIndex, BOOL bOn);
EXTERN_C DISPLAYLIB_API	BOOL DISPLIB_IsSoundOn(HDISPLIB handle, DWORD dwIndex);
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetUserContext(HDISPLIB handle, void* pUserContext);
#if defined(_USE_DXDD)
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetCallbackDDCustomDraw(HDISPLIB handle, LPDXDDCALLBACKCUSTOMDRAW lpDDCustomDraw);
#else
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetCallbackD3D9DeviceCreated(HDISPLIB handle, LPDXD9CALLBACKDEVICECREATED pd3dDeviceCreated);
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetCallbackD3D9DeviceDestroyed(HDISPLIB handle, LPDXD9CALLBACKDEVICEDESTROYED pd3dDeviceDestroyed);
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetCallbackD3D9DeviceLost(HDISPLIB handle, LPDXD9CALLBACKDEVICELOST pd3dDeviceLost);
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetCallbackD3D9DeviceReset(HDISPLIB handle, LPDXD9CALLBACKDEVICERESET pd3dDeviceReset);
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetCallbackD3D9CustomDraw(HDISPLIB handle, LPDXD9CALLBACKCUSTOMDRAW pd3dCustomDraw);
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetCallbackDispMsgProc(HDISPLIB handle, LPDISPCALLBACKMSGPROC pd3dMsgProc);
#endif
EXTERN_C DISPLAYLIB_API	void DISPLIB_UpdateTime(HDISPLIB handle, DWORD dwIndex, time_t time);
EXTERN_C DISPLAYLIB_API	time_t DISPLIB_GetUpdatedTime(HDISPLIB handle, DWORD dwIndex);
EXTERN_C DISPLAYLIB_API void DISPLIB_SetEnableChannelBackground(HDISPLIB handle, DWORD dwIndex, BOOL bEnable);
EXTERN_C DISPLAYLIB_API	BOOL DISPLIB_IsEnableChannelBackground(HDISPLIB handle, DWORD dwIndex);
EXTERN_C DISPLAYLIB_API	void DISPLIB_SetChannelBackground(HDISPLIB handle, DWORD dwIndex, D3DCOLOR color);
EXTERN_C DISPLAYLIB_API	D3DCOLOR DISPLIB_GetChannelBackground(HDISPLIB handle, DWORD dwIndex);
EXTERN_C DISPLAYLIB_API HWND DISPLIB_GetHwnd(HDISPLIB handle);

EXTERN_C DISPLAYLIB_API void DISPLIB_SetBackgroundImage(HDISPLIB handle, BOOL bEnable);
EXTERN_C DISPLAYLIB_API BOOL DISPLIB_IsEnabledBackgroundImage(HDISPLIB handle);
EXTERN_C DISPLAYLIB_API BOOL DISPLIB_SetBackgroundImageFromFile(HDISPLIB handle, LPCTSTR lpFilePath, D3DCOLOR colorKey);
EXTERN_C DISPLAYLIB_API BOOL DISPLIB_SetBackgroundImageFromMemory(HDISPLIB handle, void* pStream, UINT nLength, D3DCOLOR colorKey);
