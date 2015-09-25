#pragma once
#include <sys/timeb.h>

#ifdef LIVESESSION5_EXPORTS
#define LIVESESSION5_API __declspec(dllexport)
#else
#define LIVESESSION5_API __declspec(dllimport)
#endif

#define DEFCALL	   __cdecl

#ifdef __cplusplus
extern "C" {
#endif

typedef void*		HLIVESESSION;
typedef void*		HLIVESTREAM;


enum LiveProtocol
{
	NAUTILUS_V2					= 0,		// NC Recording Server
	NAUTILUS_V2_SETUP,
	NAUTILUS_V2_HTTP,
	NAUTILUS_V2_HTTP_SETUP,

	LIVE_RTSP_RTP_UDP			= 100,		// RTP over UDP
	LIVE_RTSP_RTP_TCP,						// RTP over RTSP over TCP
	LIVE_RTSP_RTP_TCP_HTTP,					// RTP over RTSP over HTTP over TCP
	LIVE_RTSP_RTP_UDP_MULTICAST,			// RTP over UDP Multicast

	LIVE_HITRON_EVENT_HTTP,
	LIVE_HTTP,
};

enum LIVE_MSG
{
	LIVE_NO_MSG						= 0,
	LIVE_CONNECT,
	LIVE_DISCONNECT,
	LIVE_STREAM_CONNECT,
	LIVE_STREAM_DISCONNECT,
};

enum LIVE_ERROR
{
	enLIVE_NO_ERROR					= 0,
	enLIVE_ERROR_CONNECTION_FAILED,
	enLIVE_ERROR_CONNECTION_ABORTED,
	enLIVE_ERROR_NOSTREAM,
	enLIVE_ERROR_INIT,
	enLIVE_ERROR_AUTH,
	enLIVE_ERROR_DISCONNECTED,
	enLIVE_ERROR_UNKNOWN_REQUEST,
	enLIVE_ERROR_TIMEOUT,
	enLIVE_ERROR_OTHER_LOGIN,
	enLIVE_ERROR_FAIL,
};

enum LiveFrameType
{
	FRAME_DATA = 1,
	FRAME_VIDEO_CODEC,
	FRAME_AUDIO_CODEC,
	FRAME_XML,
	FRAME_KEEP_ALIVE,
	FRAME_BINARY,
	FRAME_END_OF_BINARY,
	FRAME_END_OF_EXPORT,

	FRAME_RTSP_RTP = 100,
	FRAME_HTTP,
};

interface _IStreamReceiver5;
typedef struct _LiveNotifyMsg
{
	_IStreamReceiver5	*pReceiver;
	size_t				nIdx;
	void*				pStreamHandle;
	size_t				nMessage;
	int					nError;
	LiveProtocol		Protocol;

} LiveNotifyMsg, *LPLiveNotifyMsg;

enum StramExtType
{
	STREAM_TYPE_NRS	= 0,
	STREAM_TYPE_RTSP,
	STREAM_TYPE_MEDIA_INFO,
};

typedef struct _StreamData
{
	ULONG			Type;
	size_t			nChannel;
	const void*		pData;
	size_t			nDataSize;
	const void*		pStreamHandel;

	StramExtType	nExtensionType;
	const void*		pExtension;

	char			szContentType[64];
	struct __timeb64 tReceived;

} StreamData, *LPStreamData;

typedef enum _AV_MEDIA_TYPE
{
	AV_MEDIA_UNKNOWN = -1,
	AV_MEDIA_VIDEO,
	AV_MEDIA_AUDIO,
	AV_MEDIA_META,

} AV_MEDIA_TYPE;

typedef enum _AV_CODEC_TYPE
{
	AV_CODEC_UNKNOWN = -1,
	
	AV_CODEC_MJPEG,
	AV_CODEC_MPEG4,
	AV_CODEC_H264,

	AV_CODEC_PCM,
	AV_CODEC_G711U,
	AV_CODEC_G711A,
	AV_CODEC_G726,
	AV_CODEC_AAC,

	AV_ONVIF_META,

} AV_CODEC_TYPE;

typedef enum _RTP_VIDEO_FRAME_TYPE
{
	MPEG4_I	= 0,
	MPEG4_P,
	MPEG4_B,

	H264_SPS,
	H264_PPS,
	H264_SEI,
	H264_I,
	H264_P,
	H264_B,

} RTP_VIDEO_FRAME_TYPE;

typedef struct _RTSP_EXT_HEAD
{
	int				nCodecType;
	int				nFrameType;
	SIZE			szVideo;
	int				audio_bitrate;
	int				audio_samplerate;
	const void*		pExtData;
	size_t			nExtDataSize;

} RTSP_EXT_HEAD, *LPRTSP_EXT_HEAD;

typedef struct _AV_EXT_MEDIA_INFO
{
	AV_MEDIA_TYPE	MediaType;
	AV_CODEC_TYPE	CodecType;
	union {
		struct {
			size_t	width;
			size_t	height;
		};
		struct {
			size_t	sample_rate;
			size_t	bit_rate;
		};
	};
	const void*		pExtData;
	size_t			nExtDataSize;

} AV_EXT_MEDIA_INFO, *LPAV_EXT_MEDIA_INFO;

typedef interface _IStreamReceiver5
{
	virtual void OnNotifyMessage(LiveNotifyMsg* pNotify) = 0;
	virtual void OnReceive(LPStreamData Data) = 0;

} IStreamReceiver5;

typedef void (CALLBACK* LPLIVESESSIONMSG)(HLIVESESSION hLiveSession, LiveNotifyMsg* pNotify, void* pUserParam);
typedef void (CALLBACK* LPLIVESESSIONRECV)(HLIVESESSION hLiveSession, LPStreamData Data, void* pUserParam);

#define MAKE_LIVEVERSION(fileVer, dbVer)      ((LONG)(((WORD)(((DWORD_PTR)(fileVer)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(dbVer)) & 0xffff))) << 16))

typedef struct _LiveConnect
{
	unsigned int		nIdx;
	const char*			sAddress;
	unsigned int		nPort;
	BOOL				bAsync;

	LiveProtocol		Protocol;
	void*				pUserData;

	IStreamReceiver5*	pReceiver;

	// Can use this callback members If do not use IStreamReceiver interface, 
	LPLIVESESSIONMSG	pMsgFunc;
	void*				pMsgParam;
	LPLIVESESSIONRECV	pRecvFunc;
	void*				pRecvParam;

	const char*			pUserId;
	const char*			pUserPassword;

	// for Nautilus : MAKE_LIVEVERSION(10300, 1)
	unsigned int		nVersion;	

	// Using HTTP Proxy
	BOOL				bUseProxy;
	const char*			pProxyAddress;
	unsigned int		nProxyPort;
	const char*			pProxyId;
	const char*			pProxyPassword;

} LiveConnect;

LIVESESSION5_API HLIVESESSION	Live5_Initialize(size_t nMaxClient, size_t nThreadPoolSize);
typedef HLIVESESSION		(DEFCALL* LPLIVE5_Initialize)(size_t nMaxClient, size_t nThreadPoolSize);
LIVESESSION5_API void	Live5_Destroy(HLIVESESSION handle);
typedef void			(DEFCALL* LPLIVE5_Destroy)(HLIVESESSION handle);
LIVESESSION5_API BOOL	Live5_ConnectEx(HLIVESESSION handle, LiveConnect* conn);
typedef void			(DEFCALL* LPLIVE5_ConnectEx)(HLIVESESSION handle, LiveConnect* conn);
LIVESESSION5_API void	Live5_Disconnect(HLIVESESSION handle, size_t nChannel);
typedef void			(DEFCALL* LPLIVE5_Disconnect)(HLIVESESSION handle, size_t nChannel);
LIVESESSION5_API BOOL	Live5_IsConnected(HLIVESESSION handle, size_t nChannel);
typedef BOOL			(DEFCALL* LPLIVE5_IsConnected)(HLIVESESSION handle, size_t nChannel);

LIVESESSION5_API void	Live5_GetVideoPerSecond(HLIVESESSION handle, size_t nChannel, size_t* fps, size_t* Bps);
typedef void			(DEFCALL* LPLIVE5_GetVideoPerSecond)(HLIVESESSION handle, size_t nChannel, size_t* fps, size_t* Bps);
LIVESESSION5_API void	Live5_GetAudioPerSecond(HLIVESESSION handle, size_t nChannel, size_t* fps, size_t* Bps);
typedef void			(DEFCALL* LPLIVE5_GetAudioPerSecond)(HLIVESESSION handle, size_t nChannel, size_t* fps, size_t* Bps);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hitron Device Only
// Hitron send audio
LIVESESSION5_API BOOL	Live5_InitSendAudio(HLIVESESSION handle, size_t nChannel);
typedef BOOL			(DEFCALL* LPLIVE5_InitSendAudio)(HLIVESESSION handle, size_t nChannel);
LIVESESSION5_API int	Live5_SendAudio(HLIVESESSION handle, size_t nChannel, const void* pData, size_t nDataSize);
typedef int				(DEFCALL* LPLIVE5_SendAudio)(HLIVESESSION handle, size_t nChannel, const void* pData, size_t nDataSize);
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Nautilus Server V2
// XML must be UTF-8
LIVESESSION5_API int	Live5_SendXML(HLIVESESSION handle, size_t nChannel, const char* pXML);
typedef int				(DEFCALL* LPLIVE5_SendXML)(HLIVESESSION handle, size_t nChannel, const char* pXML);

// To connect realy, playback or event stream
LIVESESSION5_API HLIVESTREAM	Live5_ConnectStream(HLIVESESSION handle, size_t nChannel, const char* pMsg, IStreamReceiver5* pReceiver);
LIVESESSION5_API HLIVESTREAM	Live5_ConnectStream2(HLIVESESSION handle, size_t nChannel, const char* pMsg, LPLIVESESSIONMSG pMsgCallback, LPVOID pMsgParam, LPLIVESESSIONRECV pRecvCallback, LPVOID pRecvParam);
typedef HLIVESTREAM		(DEFCALL* LPLIVE5_ConnectStream2)(HLIVESESSION handle, size_t nChannel, const char* pMsg, LPLIVESESSIONMSG pMsgCallback, LPVOID pMsgParam, LPLIVESESSIONRECV pRecvCallback, LPVOID pRecvParam);
LIVESESSION5_API void Live5_DisconnectStream(HLIVESESSION handle, size_t nChannel, HLIVESTREAM stream);
typedef void			(DEFCALL* LPLIVE5_DisconnectStream)(HLIVESESSION handle, size_t nChannel, HLIVESTREAM stream);
LIVESESSION5_API BOOL	Live5_SendDataStream(HLIVESESSION handle, size_t nChannel, HLIVESTREAM stream, ULONG nDataType, const void* pData, size_t nDataSize);
typedef BOOL			(DEFCALL* LPLIVE5_SendDataStream)(HLIVESESSION handle, size_t nChannel, HLIVESTREAM stream, ULONG nDataType, const void* pData, size_t nDataSize);
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif