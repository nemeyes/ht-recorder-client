/*
Hitron Live Network Library 3.0 beta
2008.01.02
- Network 부분 모듈화

2008.02.05
- Multicast 설정시 오디오 send시 포트지정 버그 수정

2008.03.20
- TCP 일경우 RTSP media 별 ssrc 지정(서버에서 정보를 받아올 수 없음, 따라서 클라이언트에서 임의로 지정함)
  서버쪽으로 오디오정보를 보낼때 ssrc구분자가 필요함.

2008.04.16
- Support H.264 지원

2008.07.01
- Support RTP over RTSP,RTP over RTSP over HTTP

2009.07
- Upgrade 4.0

*/

#ifndef _LIVESESSIONDLL
#define _LIVESESSIONDLL
#include <winsock2.h>

#ifdef LIVESESSION_EXPORTS
#define LIVEEXT	__declspec(dllexport)
#define LIVEEXT_TEMPLATE
#else
#define LIVEEXT	__declspec(dllimport)
#define LIVEEXT_TEMPLATE extern
#endif

#define WM_LIVE_CONNECT		WM_USER	+ 70
#define WM_LIVE_DISCONNECT	WM_USER + 71

#define LIVE_NO_ERROR			0	// no error
#define LIVE_ERROR_NOSTREAM		1	// no video stream during 6 seconds
#define LIVE_ERROR_DISCONNECTED	2	// disconnected from the server or camera or PC
#define LIVE_ERROR_NORESPONSE	3	// no response keep-alive (RTSP option requests)
#define LIVE_ERROR_SOCKET		4	// socket error
#define LIVE_ERROR_CONNECTFAIL	5	// failed connection

#define LIVE_ERROR_AUTH			6	// failed authorization, 노틸러스 또는 인원초과일때도..(서버비지랑 같다)
#define LIVE_ERROR_ADMIN_OTHER_CONNECT	7
#define LIVE_ERROR_SERVERBUSY	8
#define LIVE_ERROR_HTTP			9
#define LIVE_ERROR_CONNECTION_ABORTED	10


// hitron mpeg4 user data struct
#pragma pack(push, __MPEG_USER_DATA)
#pragma pack(1)
typedef struct _MPEG_USER_DATA
{
	unsigned long nStartCode;	// 0xB2010000
	unsigned char nLength;		// 
	unsigned char nSign0;
	unsigned char nAlarm0;
	unsigned char nAlarm1;
	unsigned char nSign1;
	unsigned char nAlarm2;
	unsigned char nAlarm3;
	unsigned char nSign2;
	unsigned char nMdRes0;
	unsigned char nMdRes1;
	unsigned char nSign3;
	unsigned char nMdRes2;
	unsigned char nMdRes3;
	unsigned char nSign4;
	unsigned char nMdRes4;
	unsigned char nMdRes5;
	unsigned char nSign5;
	unsigned char nMdRes6;
	unsigned char nMdRes7;
	unsigned char nSign6;
	unsigned char nUnUsed0;
	unsigned char nUnUsed1;
	unsigned char nSign7;
} MPEG_USER_DATA;
#pragma pack(pop, __MPEG_USER_DATA)

#ifndef DEFINES_MEDIATYPE
#define DEFINES_MEDIATYPE

// 0: video, 1: audio
const BYTE AVMEDIA_TABLE[] = {
    0, 0, 1, 0, 1, 1, 1, 1, 1, 1, //0-9
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //10-19
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //20-29
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //30-39
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //40-49
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //50-59
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //60-69
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //70-79
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //80-89
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //90-99
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //100-109
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //110-119
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //120-129
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //130-139
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //140-149
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //150-159
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //160-169
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //170-179
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //180-189
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //190-199
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //200-209
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //210-219
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //220-229
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //230-239
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //240-249
    1, 1, 1, 1, 1, 1             //250-255
};

const enum AVMEDIA_TYPE
{
	AVMEDIA_MPEG4 = 0,	// MPEG4 video
	AVMEDIA_H264,		// H.264 video
	AVMEDIA_PCM,		// PCM raw audio

	AVMEDIA_JPEG,
	AVMEDIA_PCMU,	// G.711 A-law	64 kbps
	AVMEDIA_PCMA,	// G.711 u-law	64 kbps

	AVMEDIA_G726_16,
	AVMEDIA_G726_24,	// G.726 24 kbps
	AVMEDIA_G726_32,	// G.726 32 kbps
	AVMEDIA_G726_40,
	AVMEDIA_AAC,		// AAC audio (mpeg4-generic)

	AVMEDIA_HTTP	= 200,
	AVMEDIA_XML,
	AVMEDIA_EVENT_HITRON,

	AVMEDIA_NRS_META	= 400,

	AVMEDIA_UNKNOWN = 0xffffffff
};

// MEDIA_STREAM_TYPE 종류를 추가할 경우, 가장 마지막에 추가해야 함
const enum	MEDIA_STREAM_TYPE
{
	VIDEO_STREAM_TYPE = 0,
	AUDIO_STREAM_TYPE,
	INFO_STREAM_TYPE, 
};

#endif // DEFINES_MEDIATYPE

const enum LIVE_CONNECTION_METHOD
{
	METHOD_NONE = 0,

	METHOD_UDP = 1,
	METHOD_RTP_OVER_RTSP,
	METHOD_RTP_AND_RTSP_OVER_HTTP,
	METHOD_UDP_MULTICAST,		// Not support
	
	METHOD_HITRON_HTTPEVENT,	// Hitron Event Notification Stream
	METHOD_HTTP,

	METHOD_NAUTILUS = 10,		// connect to nautilus server
	METHOD_NAUTILUSTUNNEL,		// connect to nautilus server to use tcp over http
	METHOD_NAUTILUS_CONTROL,	// connect to nautilus server for only server control (over http)
	METHOD_NAUTILUS_EVENT,

	// IDIS INT1000 관련
	METHOD_INT1000	= 20,
	// IDIS DOME 관련
	METHOD_DOME		= 21,


	// SONY 카메라 관련
	METHOD_SONY_TCP	= 30,		// Not Support
	METHOD_SONY_UDP = 31, 
	METHOD_SONY_UDP_MULTICAST = 32,	// Not Supoort

};

#define CODING_TYPE_INTRA		0x00
#define CODING_TYPE_PREDICTIVE	0x01
#define CODING_TYPE_BIDIRECTION	0x02
#define CODING_TYPE_SPRITE		0x03

#define CODED_IDR				10
#define CODED_NON_IDR			11
#define CODED_PARTITION_A		12
#define CODED_SEI				13
#define CODED_SPS				14
#define CODED_PPS				15

// GRSTREAM_BUFFER : eventType 에 설정함. event종류 8개 정의함. 현재는 2개
#define GRS_EVENT_ALARM			0x01
#define GRS_EVENT_MOTION		0x02

typedef struct _AVMEDIA_INFO
{
	AVMEDIA_TYPE mediaType;
	union {
		struct {
			u_int width;
			u_int height;
		};
		struct {
			u_int sample_rate;
			u_int bit_rate;
		};
	};

	BYTE	*pExtra;
	DWORD	dwExtra;

} AVMEDIA_INFO;

typedef struct _AVBUFFER
{
	DWORD dwChannel;
	AVMEDIA_TYPE mediaType;
	MEDIA_STREAM_TYPE	streamType;
	unsigned char *pBuffer;
	size_t nLength;
	DWORD dwTickCount;
	unsigned long timestamp;

	// for video info
	unsigned int nWidth;
	unsigned int nHeight;

	// for audio info
	unsigned int sr;
	unsigned int br;
	
	int nFrameType;

	// received packet time
	__int64 tReceivedTime;	// time_t -> 64비트로
	struct timeval tv;

	// H.264 or AAC config data
	unsigned char *pExtraData;
	size_t nExtraData;

	// RTP Extention Data
	int nExtentionType;
	unsigned char *pExtentionData;
	size_t nExtentionDataSize;

} AVBUFFER, *LPAVBUFFER;

// AVBUFFER에 mac이 없어서 따로 정의함.
struct _GRSTREAM_BUFFER : public AVBUFFER
{
	tm		playback_time;	// playback time
	unsigned int	cam_source_index;

	unsigned char mac[8];	// mac address hex 6bytes
	char	panic;			// 스트림이 중간에 짤릴때, 클라이언트에서 panic 처리하기 위함
	char	gr_type;		// 0: live, 1: playback, 2: clip data, 3: clip complete, 4: event info
	u_char	eventType;		// 해당 스트림에 이벤트 유/무 0x01 => Alarm, 0x02 = > Motion
	u_char	bH264_New_Cam;		// H.264 New Camera 유무
	u_char	reserve0[4];
};
typedef struct _GRSTREAM_BUFFER GRSTREAM_BUFFER, *LPGRSTREAM_BUFFER;

typedef interface _IStreamReceiver
{
	virtual void GetFrame(AVBUFFER *pBuffer) = 0;
} IStreamReceiver;

interface _IStreamReceiver2 : public IStreamReceiver
{
	virtual void OnRecvGRMsg(DWORD dwChannel, void *pHead, void *pData, DWORD dwDataLen) = 0;
};
typedef _IStreamReceiver2 IStreamReceiver2;

typedef interface _IRecorder
{
	virtual void GetRecordFrame(GRSTREAM_BUFFER *pBuffer) = 0;
} IRecorder;

typedef struct _LiveConnectionT
{
	DWORD					dwChannel;
	const char				*lpszAddress;
	UINT					unPort;
	LIVE_CONNECTION_METHOD	method;
	IStreamReceiver2		*pStreamReceiver;
	const char				*pUserId;
	const char				*pPassword;
	BOOL					bAsyncronous;
	HWND					hSendMessage;
	DWORD					dwThreadID;
	DWORD					dwModel;	// not used
	BOOL					bSSL;	// If method is METHOD_HTTP. However it does not support yet.
	char					mac[8];	// 6byte mac주소: 2byte는 padding

	LPARAM					lParam;

} LiveConnectionT, *LPLiveConnectionT;

struct LiveConnectionT2 : public LiveConnectionT
{
	const char				*pProxyAddress;
	UINT					nProxyPort;
	const char				*pProxyUser;
	const char				*pProxyPassword;
};

#ifdef __cplusplus
extern "C" {
#endif

LIVEEXT void* Live_Initialize(DWORD dwMaxChannel, DWORD dwNumberOfThread = 0);
LIVEEXT void Live_Destroy(void* handle);
LIVEEXT BOOL Live_Connect(void* handle, const char *lpszAddress, u_short unPort, DWORD dwChannel,
		IStreamReceiver *pStreamReceiver, HWND hSendMessage = NULL,
		const char *pUserId = NULL, const char *pPassword = NULL,
		LIVE_CONNECTION_METHOD method = METHOD_UDP,
		BOOL bAsynchronous = FALSE,
		DWORD dwThreadID = 0);
LIVEEXT BOOL Live_ConnectEx(void* handle, LiveConnectionT *pLiveConnection);
LIVEEXT BOOL Live_ConnectEx2(void* handle, LiveConnectionT2 *pLiveConnection);
LIVEEXT void Live_Disconnect(void* handle, DWORD dwChannel);

LIVEEXT BOOL Live_InitializeSendAudio(void* handle, DWORD dwChannel, WORD nBitsPerSample);
LIVEEXT void Live_SendAudio(void* handle, DWORD dwChannel, char *pData, DWORD dwDataLen);

//LIVEEXT void Live_SetRecorder(void* handle, IRecorder *pRecorder);
LIVEEXT void Live_GetVideoInfo(void* handle, DWORD dwChannel, unsigned short *fps, unsigned long *bps);
LIVEEXT BOOL Live_GetMediaInfo(void* handle, DWORD dwChannel, MEDIA_STREAM_TYPE type, AVMEDIA_INFO *lpInfo);

LIVEEXT const char* Live_GRGetCameraInfo(void* handle, DWORD dwChannel);
LIVEEXT int Live_SendGRMsg(void* handle, DWORD dwChannel, int nCommand, void* pData, DWORD dwDataLen, BOOL bNonBlock);
LIVEEXT int Live_RecvGRMsg(void* handle, DWORD dwChannel, void* pHead, void* pData, DWORD dwDataLen, DWORD dwTimeout);
LIVEEXT int Live_SendGRStream(void* handle, DWORD dwChannel, void* pInfo, LPVOID pData, DWORD dwDataLen);

LIVEEXT void Live_SetItemData(void* handle, DWORD dwChannel, LPARAM lParam);
LIVEEXT LPARAM Live_GetItemData(void* handle, DWORD dwChannel);

LIVEEXT BOOL Live_SendRTSPGetParameter(void* handle, DWORD dwChannel, LPVOID lpData, DWORD dwDataSize);
LIVEEXT BOOL Live_SendRTSPSetParameter(void* handle, DWORD dwChannel, LPVOID lpData, DWORD dwDataSize);

LIVEEXT BOOL Live_IsConnected(void* handle, DWORD dwChannel);
LIVEEXT void Live_SetNoCheckKeepAlive(void *handle, DWORD dwChannel, BOOL bEnable);

#ifdef __cplusplus
}
#endif

#endif // _LIVESESSIONDLL