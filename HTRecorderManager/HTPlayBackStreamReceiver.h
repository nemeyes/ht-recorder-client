#pragma once

#include <HTRecorderDLL.h>
#include <LiveSessionDLL.h>
#include <LiveSession5.h>
#include <queue>

class HTPlayBackStreamReceiver : public IPlayBackStreamReceiver
{
public:
	HTPlayBackStreamReceiver(HTRecorder * service);
	~HTPlayBackStreamReceiver(VOID);

	virtual VOID OnNotifyMessage( LiveNotifyMsg * pNotify );
	virtual VOID OnReceive( LPStreamData Data );

	UINT GetPlaybackID() { return m_nPlaybackID; }

	VOID AddStreamInfo( INT32 channel, CString uuid );

	VOID RemoveStreamInfo( INT32 channel );

	VOID RemoveStreamInfo( CString uuid );
	VOID RemoveAllStreamInfo( VOID );
	BOOL IsConnected(VOID) CONST{ return m_bConntected; }
	HTRecorder* GetRecorder(VOID) { return m_HTReocrder; }
	VOID Start( VOID );
	VOID Stop( VOID );

private:
	VOID GetTime( UINT date, UINT time, UINT& year, UINT& month, UINT& day, UINT& hour, UINT& minute, UINT& second );
	VOID InitPbInfo( VOID );
	VOID InsertPbInfo( int nIdx, AVMEDIA_TYPE avt_Type, UINT32 ui32Width, UINT32 ui32Height );
	
	HTRecorder *					m_HTReocrder;
	UINT							m_nPlaybackID;
	std::map<INT32,CString>			m_pbUUIDs;
	RS_PLAYBACK_STREAM_INFO_T		m_pbInfo[RS_MAX_PLAYBACK_CH];
	CCriticalSection				m_lock;
	BOOL							m_bConntected;
	BOOL							m_bRun;

	BYTE *							m_pVideoExtraData;
	int								m_nVideoExtraDataSize;

	BYTE *							m_pAudioExtraData;
	int								m_nAudioExtraDataSize;
//	HTRECORD_TIME_T					m_playbackTime;

	AVMEDIA_TYPE					m_nVideoType;
	AVMEDIA_TYPE					m_nAudioType;

};
