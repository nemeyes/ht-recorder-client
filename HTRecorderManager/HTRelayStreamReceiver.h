#pragma once

#include <HTRecorderDLL.h>
#include <LiveSessionDLL.h>
#include <LiveSession5.h>
#include <queue>
#include "DisplayLib.h"

class HTRelayStreamReceiver : public IRelayStreamReceiver
{
public:
	HTRelayStreamReceiver(HTRecorder * service, CString strCameraUuid, CDisplayLib * pVideoView, unsigned char * key, size_t nChannel);
	~HTRelayStreamReceiver(VOID);

	virtual VOID OnNotifyMessage(LiveNotifyMsg * pNotify);
	virtual VOID OnReceive(LPStreamData Data);

	CString GetStreamInfo( VOID ) { return m_relayUUID; }

	BOOL IsConnected( VOID ) CONST { return m_bConntected; }
	HTRecorder * GetRecorder(VOID) { return m_HTRecorder; }

	VOID Start( VOID );
	VOID Stop( VOID );
private:
	VOID GetTime( UINT date, UINT time, UINT& year, UINT& month, UINT& day, UINT& hour, UINT& minute, UINT& second );

	HTRecorder * m_HTRecorder;
	CString m_relayUUID;
	RS_RELAY_STREAM_INFO_T m_relayInfo;
	BOOL m_bConntected;
	BOOL m_bRun;


	/*BYTE * m_pVideoExtraData;
	int m_nVideoExtraDataSize;

	BYTE * m_pAudioExtraData;
	int m_nAudioExtraDataSize;*/

	AVMEDIA_TYPE m_nVideoType;
	AVMEDIA_TYPE m_nAudioType;


	CDecoder m_Decode;
	CDisplayLib * m_pVideoView;
	int m_nVideoViewChannels;

	BOOL m_bFindFirstKey;

	BYTE m_key[8];
	char m_strKey[200];

};
