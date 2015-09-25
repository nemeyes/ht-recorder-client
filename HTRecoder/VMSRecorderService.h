// VMSRecorderService.h : VMSRecorderService DLL의 기본 헤더 파일입니다.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH에 대해 이 파일을 포함하기 전에 'stdafx.h'를 포함합니다."
#endif

#include "XMLParser.h"
class HTRecorder;
class ServiceCore : public IStreamReceiver5//, public CMessageSender
{
public:
	ServiceCore( size_t id, LPCTSTR serverID, VOID *liveSession, HTRecorder *recorder );
	virtual ~ServiceCore( VOID );

	//base functionalities
	virtual void	OnNotifyMessage( LiveNotifyMsg* notify );
	virtual void	OnReceive( LPStreamData Data );
	virtual BOOL	Connect( CONST CString& strAddress=_T("127.0.0.1"), 
							 CONST UINT nPort=DEFAULT_NCSERVICE_PORT, 
							 CONST CString& strUser=_T("admin"), 
							 CONST CString& strPassword=_T("admin"), 
							 LiveProtocol protocol=NAUTILUS_V2, 
							 ULONG fileVersion=NAUTILUS_FILE_VERSION, 
							 ULONG dbVersion=NAUTILUS_DB_VERSION, 
							 BOOL bAsync=TRUE );

	virtual BOOL	Reconnect( VOID );
	virtual VOID	Disconnect( VOID );

	VOID			SetProtocol( LiveProtocol protocol );
	VOID			SetAsyncrousConnection( BOOL bAsync );
	LiveProtocol	GetProtocol( VOID ) CONST;
	BOOL			IsAsyncrousConnection() CONST;

	BOOL			IsConnected( BOOL bNoSync=FALSE );
	BOOL			IsConnecting( BOOL bNoSync=FALSE );
	VOID			SetAddress( CONST CString& strAddress );
	VOID			SetPort( CONST UINT nPort );
	VOID			SetUser( CONST CString& strUserId, CONST CString& strUserPassword );
	CString			GetAddress( VOID ) CONST { return _strAddress; }
	UINT			GetPort( VOID ) CONST { return _nPort; }
	CString			GetUserId( VOID ) CONST;
	CString			GetUserPassword( VOID ) CONST;

	size_t			GetClientID( VOID ) CONST;
	size_t			GetID( VOID ) CONST;
	VOID			SetServerID( LPCTSTR lpServerID );
	CString			GetServerID( VOID ) CONST;

	INT32			SendXML( CONST WCHAR* pXML );
	INT32			SendXML( CONST CHAR* pXML );
	INT32			SendXML( CONST WCHAR* pXML, LPCONTROL_RECEIVEDATAFUNCTION Func, VOID* pUserContext );
	INT32			SendXML( CONST CHAR* pXML, LPCONTROL_RECEIVEDATAFUNCTION Func, VOID* pUserContext );

	INT32			SendRecvXML( CONST WCHAR* pSendXML, std::string& sRecv, DWORD dwTimeout=3000 );
	INT32			SendRecvXML( CONST CHAR* pSendXML, std::string& sRecv, DWORD dwTimeout=3000 );
	INT32			SendRecvXML( std::string& sSend, std::string& sRecv, DWORD dwTimeout=3000 );

	VOID			AddNotifyCallback( LPCTSTR pNodeName, LPCONTROL_RECEIVEDATAFUNCTION Func, VOID* pUserContext );
	VOID			RemoveNotifyCallback( LPCTSTR pNodeName );

	///////////////// LOGIN
	BOOL			KeepAliveRequest( VOID );

	///////////////// DEVICE
	BOOL			GetDeviceList( RS_DEVICE_INFO_SET_T *deviceInfoList ); 	//서버에 등록된 장치 목록 가져오기
	BOOL			CheckDeviceStatus( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_DEVICE_STATUS_SET_T *deviceStatusList ); //장치 Status 정보 요청
	BOOL			UpdateDevice( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_DEVICE_RESULT_STATUS_SET_T *deviceResultStatusList ); //장치 등록, 수정
	BOOL			RemoveDevice( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_DEVICE_RESULT_STATUS_SET_T *deviceResultStatusList ); //장치 삭제
	BOOL			RemoveDeviceEx( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_DEVICE_RESULT_STATUS_SET_T *deviceResultStatusList ); //장치 삭제


	///////////////// RELAY STREAM
	BOOL			GetRelayInfo( VOID *xmlData, RS_RELAY_INFO_T *rlInfo );
	BOOL			StartRelay( RS_DEVICE_INFO_T *relayDevice, RS_RELAY_REQUEST_T *rlRequest );
	BOOL			UpdateRelay( RS_DEVICE_INFO_T *relayDevice, RS_RELAY_REQUEST_T *rlRequest );
	BOOL			StopRelay( RS_RELAY_INFO_T *rlInfo );

	///////////////// RECORDNIG
	BOOL			GetRecordingScheduleList( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_RECORD_SCHEDULE_SET_T *recordShcedList );
	BOOL			UpdateRecordingSchedule( RS_RECORD_SCHEDULE_SET_T *recordSchedList, RS_RESPONSE_INFO_SET_T *responseInfoList );
	BOOL			SetRecordingOverwrite( RS_RECORD_OVERWRITE_INFO_T *overwriteInfo, RS_RESPONSE_INFO_T *responseInfo );
	BOOL			GetRecordingOverwrite( RS_RECORD_OVERWRITE_INFO_T *overwriteInfo );
	BOOL			SetRecordingRetentionTime( RS_RECORD_RETENTION_INFO_T *retentionInfo, RS_RESPONSE_INFO_T *responseInfo );
	BOOL			GetRecordingRetentionTime( RS_RECORD_RETENTION_INFO_T *retentionInfo );
	BOOL			GetDiskInfo( RS_DISK_INFO_SET_T *diskInfoList );
	BOOL			ReserveDiskSpace( RS_DISK_INFO_SET_T *diskInfoList, RS_DISK_RESPONSE_SET_T *diskResponseInfoList );
	BOOL			GetDiskPolicy( RS_DISK_POLICY_SET_T *diskPolicyList );
	BOOL			UpdateDiskPolicy( RS_DISK_INFO_SET_T *diskInfo, RS_DEVICE_INFO_SET_T *deviceInfoList, RS_DISK_RESPONSE_SET_T *diskResponseList );
	BOOL			IsRecording( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_RECORDING_STATUS_SET_T *recordingStatusList );
	BOOL			StartRecordingRequest( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_RESPONSE_INFO_SET_T *responseInfoList );
	BOOL			StopRecordingRequest( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_RESPONSE_INFO_SET_T *responseInfoList );
	BOOL			StartRecordingAll( VOID );
	BOOL			StopRecordingAll( VOID );
	BOOL			StartManualRecording( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_RESPONSE_INFO_SET_T *resposneInfoList );
	BOOL			StopManualRecording( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_RESPONSE_INFO_SET_T *responseInfList );
	BOOL			DeleteRecordingData( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_DELETE_RESPONSE_SET_T *deleteResponseList );

	///////////////// PLAYBACK STREAM
	//CALENDAR SEARCH
	BOOL			GetYearIndex( RS_PLAYBACK_DEVICE_SET_T *pbDeviceList, INT year/*, CBitArray *monthList*/ );
	BOOL			GetMonthIndex( RS_PLAYBACK_DEVICE_SET_T *pbDeviceList, INT year, INT month/*, CBitArray *dayList*/ );
	BOOL			GetDayIndex( RS_PLAYBACK_DEVICE_SET_T *pbDeviceList, INT year, INT month, INT day/*, CBitArray *hourList*/ );
	BOOL			GetHourIndex( RS_PLAYBACK_DEVICE_SET_T *pbDeviceList, INT year, INT month, INT day, INT hour/*, CBitArray *minuteList, CBitArray *dupMinuteList*/ );
	//PLAYBACK
	BOOL			StartPlayback( RS_PLAYBACK_DEVICE_SET_T *pbDeviceList, RS_PLAYBACK_REQUEST_T *pbRequest, RS_PLAYBACK_INFO_T *pbInfo );
	BOOL			GetPlaybackInfo( VOID *xmlData, RS_PLAYBACK_INFO_T *pbInfo );
	BOOL			StopPlayback( RS_PLAYBACK_INFO_T *pbInfo );
	//PLAYBACK CONTROL
	BOOL			ControlPlay( RS_PLAYBACK_INFO_T *pbInfo );
	BOOL			ControlFowardPlay( RS_PLAYBACK_INFO_T *pbInfo );
	BOOL			ControlBackwardPlay( RS_PLAYBACK_INFO_T *pbInfo );
	BOOL			ControlStop( RS_PLAYBACK_INFO_T *pbInfo );
	BOOL			ControlPause( RS_PLAYBACK_INFO_T *pbInfo );
	BOOL			ControlResume( RS_PLAYBACK_INFO_T *pbInfo );
	BOOL			ControlJump( RS_PLAYBACK_JUMP_REQUEST_T *pbRequest, RS_PLAYBACK_INFO_T *pbInfo );
	BOOL			ControlGoToFirst( RS_PLAYBACK_INFO_T *pbInfo, RS_PLAYBACK_GOTO_FIRST_RESPONSE_T *pbResponse );
	BOOL			ControlGoToLast( RS_PLAYBACK_INFO_T *pbInfo, RS_PLAYBACK_GOTO_LAST_RESPONSE_T *pbResponse );

	BOOL			ControlForwardStep( RS_PLAYBACK_INFO_T *pbInfo );
	BOOL			ControlBackwardStep( RS_PLAYBACK_INFO_T *pbInfo );

	///////////////// EXPORT
	BOOL			StartExport( RS_DEVICE_INFO_SET_T *devInfoList, RS_EXPORT_REQUEST_T *expRequest, RS_EXPORT_RESPONSE_T *expResponse );
	BOOL			StopExport( RS_EXPORT_INFO_T *expInfo );
	BOOL			PauseExport( RS_EXPORT_INFO_T *expInfo );
	BOOL			ResumeExport( RS_EXPORT_INFO_T *expInfo );


	//////////////// Server Notification
	VOID			OnConnectionStop( RS_CONNECTION_STOP_NOTIFICATION_T *notification );
	VOID			OnRecordingStorageFull( RS_STORAGE_FULL_NOTIFICATION_T *notification );
	VOID			OnReservedStorageFull( RS_STORAGE_FULL_NOTIFICATION_T *notification );
	VOID			OnOverwritingError( RS_OVERWRITE_ERROR_NOTIFICATION_T *notification );
	VOID			OnConfigurationChanged( RS_CONFIGURATION_CHANGED_NOTIFICATION_T *notification );
	VOID			OnPlaybackError( RS_PLAYBACK_ERROR_NOTIFICATION_T *notification );
	VOID			OnDiskError( RS_DISK_ERROR_NOTIFICATION_T *notification );
	VOID			OnKeyFrameMode( RS_KEY_FRAME_MODE_NOTIFICATION_T *notification );
	VOID			OnBufferClean( RS_BUFFER_CLEAN_NOTIFICATION_T *notification );

protected:
	VOID*	GetHandle( VOID );
	//extension
	VOID	CheckStream( DWORD dwFlags );
	BOOL	CheckSafeStreamRelease( VOID );
		
	VOID	RemoveDiskAll( VOID );

	// LOGIN //
	//BOOL	GetDevice( std::tr1::shared_ptr<RS_DEVICE_INFO_T>& DevicePtr, LPCTSTR lpDeviceID );

private:
	BOOL			CheckErrorCallBack( CONST CHAR *pRecvStr );
	RS_ERROR_TYPE	CheckRSErrorCallBack( MSXML2::IXMLDOMNodePtr pNRSError );


	///////////////// DEVICE
	BOOL			LoadDevicesCallBack( MSXML2::IXMLDOMNodeListPtr pList, RS_DEVICE_INFO_SET_T *deviceInfoList );
	BOOL			CheckDeviceStatusCallBack( CONST CHAR *pXML, RS_DEVICE_STATUS_SET_T *deviceStatusList );
	BOOL			CommonDeviceCallBack( CONST CHAR *pXML,  RS_DEVICE_RESULT_STATUS_SET_T *deviceResultStatusList, LPCTSTR strCheck );

	///////////////// RECORDNIG
	BOOL			LoadScheduleListCallBack( CONST CHAR *pXML, RS_RECORD_SCHEDULE_SET_T *recordShcedList );
	BOOL			UpdateScheduleResultCallBack( CONST CHAR *pXML, RS_RESPONSE_INFO_SET_T *responseInfoList, LPCTSTR strCheck );
	BOOL			OverwriteResultCallBack( CONST CHAR *pRecvStr, RS_RESPONSE_INFO_T *responseInfo );
	BOOL			GetOverwriteResultCallBack( CONST CHAR *pRecvStr, RS_RECORD_OVERWRITE_INFO_T *overwriteInfo );
	BOOL			UpdateRetentionResultCallBack( CONST CHAR *pRecvStr, RS_RESPONSE_INFO_T *responseInfo );
	BOOL			GetRetentionResultCallBack( CONST CHAR *pRecvStr, RS_RECORD_RETENTION_INFO_T *retentionInfo );
	BOOL			LoadDisksCallBack( CONST CHAR *pXML, RS_DISK_INFO_SET_T *diskInfoList );
	BOOL			ReserveDiskResultCallBack( CONST CHAR *pXML, RS_DISK_RESPONSE_SET_T *diskResponseInfoList, LPCTSTR strCheck );
	BOOL			LoadDiskPolicyCallBack( CONST CHAR * pXML, RS_DISK_POLICY_SET_T *diskPolicyList );
	BOOL			DiskPolicyResultCallBack( CONST CHAR * pXML, RS_DISK_RESPONSE_SET_T *diskResponseList, LPCTSTR strCheck );
	BOOL			IsRecordingCallBack( CONST CHAR * pXML, RS_RECORDING_STATUS_SET_T *recordingStatusList );
	BOOL			RecordingResultCallBack( CONST CHAR * pXML, RS_RESPONSE_INFO_SET_T *responseInfoList );
	BOOL			DeleteDataResultCallBack( CONST CHAR * pXML, RS_DELETE_RESPONSE_SET_T *deleteResponseList, LPCTSTR strCheck );



	///////////////// PLAYBACK STREAM
	//CALENDAR SEARCH
	BOOL			GetYearIndexCallBack( CONST CHAR * pXML, RS_PLAYBACK_DEVICE_SET_T *pbDeviceList/*, CBitArray *monthList*/ );
	VOID			SetMonthIndex( LPCTSTR deviceID, CONST CHAR *pIndexBase64, RS_PLAYBACK_DEVICE_SET_T *pbDeviceList/*, CBitArray *monthList*/ );

	BOOL			GetMonthIndexCallBack( CONST CHAR * pXML, RS_PLAYBACK_DEVICE_SET_T *pbDeviceList/*, CBitArray *dayList*/ );
	VOID			SetDayIndex( LPCTSTR deviceID, CONST CHAR *pIndexBase64, RS_PLAYBACK_DEVICE_SET_T *pbDeviceList/*, CBitArray *dayList*/ );

	BOOL			GetDayIndexCallBack( CONST CHAR * pXML, RS_PLAYBACK_DEVICE_SET_T *pbDeviceList/*, CBitArray *hourList*/  );
	VOID			SetHourIndex( LPCTSTR deviceID, CONST CHAR *pIndexBase64, RS_PLAYBACK_DEVICE_SET_T *pbDeviceList/*, CBitArray* hourList*/ );

	BOOL			GetHourIndexCallBack( CONST CHAR * pXML, RS_PLAYBACK_DEVICE_SET_T *pbDeviceList/*, CBitArray *minuteList, CBitArray *dupMinuteList*/ );
	VOID			SetMinIndex( LPCTSTR deviceID, CONST CHAR *pIndexBase64, /*CONST CHAR *pDupIndexBase64, */RS_PLAYBACK_DEVICE_SET_T *pbDeviceList/*, CBitArray *minuteList, CBitArray *dupMinuteList*/ );

	BOOL			ControlStopCallBack( CONST CHAR *pRecvStr, RS_PLAYBACK_INFO_T *pbInfo );
	BOOL			GoToFirstCallBack( CONST CHAR * pXML, RS_PLAYBACK_GOTO_FIRST_RESPONSE_T *pbResponse );
	BOOL			GoToLastCallBack( CONST CHAR * pXML, RS_PLAYBACK_GOTO_LAST_RESPONSE_T *pbResponse );


protected:
	//base attributes
	size_t				_nId;
	CString				_strAddress;
	UINT				_nPort;
	CString				_strUserId;
	CString				_strUserPassword;
	BOOL				_bIsConnecting;
	BOOL				_bIsConnected;
	XMLParser		   *_parser;

	// Block 모드에서 req_id 정보를 이용해서 동작함.
	HANDLE				_hEvent;
	CString				_strReqID;
	BOOL				_bBlockMode;
	std::string			_sRecvStr;

	LiveProtocol		_protocol;
	BOOL				_bAsyncrousConnection;

	CRITICAL_SECTION	_cs;
	CRITICAL_SECTION	_csCallback;
	std::map<std::wstring,CONTROL_RECEIVEDATAFUNCTION_T>	_controlCallbackList;

	VOID			   *_liveSession;
	CString				_strServerID;


	HTRecorder		   *_exposedService;

};