#pragma once

#include <RecorderDLL.h>
#include <LiveSessionDLL.h>
#include <LiveSession5.h>
#include <queue>

class RelayStreamReceiver : public IRelayStreamReceiver
{
public:
	RelayStreamReceiver( Recorder * service );
	~RelayStreamReceiver( VOID );

	virtual VOID OnNotifyMessage( LiveNotifyMsg* pNotify );
	virtual VOID OnReceive( LPStreamData Data );

	VOID SetStreamInfo( CString UUID );
	CString GetStreamInfo( VOID ) { return _relayUUID; }

	BOOL IsConnected( VOID ) CONST { return _bConntected; }
	Recorder* GetRecorder( VOID ) { return _service; }

	VOID Start( VOID );
	VOID Stop( VOID );

private:
	VOID GetTime( UINT date, UINT time, UINT& year, UINT& month, UINT& day, UINT& hour, UINT& minute, UINT& second );

	Recorder *						_service;
	CString							_relayUUID;
	RS_RELAY_STREAM_INFO_T			_relayInfo;
	CRITICAL_SECTION				_lock;
	BOOL							_bConntected;
	BOOL							_bRun;
};
