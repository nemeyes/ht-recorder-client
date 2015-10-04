#pragma once

#include <RecorderDLL.h>
#include <LiveSessionDLL.h>
#include <LiveSession5.h>
#include <queue>
 /*!
	\brief PlayBack Receiver Class for Hitron Recorder Server
*/
class PlayBackStreamReceiver : public IPlayBackStreamReceiver
{
public:
	PlayBackStreamReceiver( Recorder * service );
	~PlayBackStreamReceiver( VOID );

	/*!
		\brief CallBack function for Message pushing by Recorder Server.
		\param pNotify : Recorder Server Notify Message.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	virtual VOID OnNotifyMessage( LiveNotifyMsg* pNotify );

	/*!
		\brief Get Recorder IP Address using UUID.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	virtual VOID OnReceive( LPStreamData Data );
		
	/*!
		\brief Get Recorder IP Address using UUID.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	UINT GetPlaybackID() { return _playbackID; }

	/*!
		\brief Get Recorder IP Address using UUID.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	VOID AddStreamInfo( INT32 channel, CString uuid );

	/*!
		\brief Get Recorder IP Address using UUID.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	VOID RemoveStreamInfo( INT32 channel );

	/*!
		\brief Get Recorder IP Address using UUID.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	VOID RemoveStreamInfo( CString uuid );

	/*!
		\brief Get Recorder IP Address using UUID.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	VOID RemoveAllStreamInfo( VOID );

	/*!
		\brief Get Recorder IP Address using UUID.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	BOOL IsConnected( VOID ) CONST { return _bConntected; }

	/*!
		\brief Get Recorder IP Address using UUID.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	Recorder* GetRecorder( VOID ) { return _service; }

	/*!
		\brief Get Recorder IP Address using UUID.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	VOID Start( VOID );

	/*!
		\brief Get Recorder IP Address using UUID.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	VOID Stop( VOID );

private:
	/*!
		\brief Get Recorder IP Address using UUID.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	VOID GetTime( UINT date, UINT time, UINT& year, UINT& month, UINT& day, UINT& hour, UINT& minute, UINT& second );

	/*!
		\brief Get Recorder IP Address using UUID.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	VOID InitPbInfo( VOID );

	/*!
		\brief Get Recorder IP Address using UUID.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	VOID InsertPbInfo( int nIdx, AVMEDIA_TYPE avt_Type, UINT32 ui32Width, UINT32 ui32Height );


	Recorder *						_service;
	UINT							_playbackID;
	std::map<INT32,CString>			_pbUUIDs;
	RS_PLAYBACK_STREAM_INFO_T		_pbInfo[RS_MAX_PLAYBACK_CH];
	CRITICAL_SECTION				_lock;
	BOOL							_bConntected;
	BOOL							_bRun;

	RECORD_TIME	m_playbackTime;

};
