#include "Common.h"
#include "HTRecorderDLL.h"
#include "ServiceCoordinator.h"
#include "VMSRecorderService.h"
#include <string_helper.h>
#include "HTDateTime.h"
#include "XML.h"
#include "MovieMaker.h"

ExportStreamReceiver::ExportStreamReceiver( VOID ) 
	: _nExportID(-1)
	, _pMsgHandler(NULL)
{
	_clsMaker = new MovieMaker( MOVIE_FILE_FORMAT_AVI );

	HINSTANCE self;
#if defined(_DEBUG)
	self = ::GetModuleHandle( _T("HTRecorderD.dll") );
#else
	self = ::GetModuleHandle( _T("HTRecorder.dll") );
#endif
	TCHAR szModuleName[MAX_PATH] = {0};
	TCHAR szModulePath[FILENAME_MAX] = {0};
	TCHAR *pszModuleName = szModulePath;
	pszModuleName += GetModuleFileName( self, pszModuleName, (sizeof(szModulePath)/sizeof(*szModulePath))-(pszModuleName-szModulePath) );
	if( pszModuleName!=szModulePath )
	{ 
		TCHAR *slash = _tcsrchr( szModulePath, _T('\\') );
		if( slash!=NULL )
		{
			pszModuleName = slash+1;
			_tcsset( pszModuleName, 0 );
		}
		else
		{
			_tcsset( szModulePath, 0 );
		}
	}
	_stprintf( szModuleName, _T("%s%s"), szModulePath, _T("MovieMaker.dll") );
	_clsMaker->Load( szModuleName );
}

ExportStreamReceiver::~ExportStreamReceiver( VOID )
{
	Stop();
	delete _clsMaker;
}


int ExportStreamReceiver::Start( CTime tStart, CTime tEnd, CString strFilePath )
{
	_tStart = tStart;
	_tEnd	= tEnd;
	return _clsMaker->Start(strFilePath);
}

int ExportStreamReceiver::Stop( VOID )
{
	return _clsMaker->Stop();
}

void ExportStreamReceiver::OnNotifyMessage( LiveNotifyMsg* pNotify )
{
	int a = 2;
}

void ExportStreamReceiver::OnReceive( LPStreamData Data )
{
	if( !_clsMaker->IsStarted() ) return;

	if( _clsMaker->AddStreamData(Data)<0 )
	{
		if( _pMsgHandler )
		{
			_pMsgHandler( RS_EP_MESSAGE_WRITE_FAILED, NULL, _pHandlerParam );
		}
	}

	switch(Data->Type)
	{
	case FRAME_XML:
		{
			CXML xml;
			string sFrameXML((const char*)Data->pData, Data->nDataSize);
			if(xml.LoadXMLFromString(sFrameXML.c_str()))
			{
				MSXML2::IXMLDOMNodePtr pExportId = xml.FindNode(_T("//ExportID"));
				if(pExportId){
					_nExportID = _ttoi((LPCTSTR)pExportId->Gettext());
				}
			}

		}
		break;
	case FRAME_VIDEO_CODEC:
		{

		}
		break;
	case FRAME_AUDIO_CODEC:
		{

		}
		break;
	case FRAME_END_OF_EXPORT:
		{
			_clsMaker->Stop();
			if( _pMsgHandler )
			{
				_pMsgHandler( RS_EP_MESSAGE_FINISHED, NULL, _pHandlerParam );
			}
		}
		break;
	case FRAME_DATA:
		{
			RS_FRAME_HEADER_T* pHeader = (RS_FRAME_HEADER_T*)Data->pData;
			CHTDateTime tTime(pHeader->date, pHeader->time);

			//TRACE(L"[ExportStreamReceiver::OnReceive] INFO - Frame Data (%u %u)\n", pHeader->date, pHeader->time);

			UINT id_rate=0;
			UINT nProgress = (UINT)((100 * (tTime.GetTime() - _tStart.GetTime())) / (_tEnd.GetTime() - _tStart.GetTime()));
			id_rate|=(UINT)(_nExportID<<16);
			id_rate|=nProgress;
			if( _pMsgHandler )
			{
				_pMsgHandler( RS_EP_MESSAGE_PROGRESS, (LPVOID)id_rate, _pHandlerParam );
			}
			TRACE(L"[ExportStreamReceiver::OnReceive] INFO - (id:%d  rate:%u)\n", _nExportID, nProgress);
		}
		break;
	}
}

VOID* IRelayStreamReceiver::GetHandle( VOID ) 
{ 
	return _handle; 
}

VOID IRelayStreamReceiver::SetHandle( VOID *handle ) 
{ 
	_handle = handle; 
}

VOID* IPlayBackStreamReceiver::GetHandle( VOID ) 
{ 
	return _handle; 
}

VOID IPlayBackStreamReceiver::SetHandle( VOID *handle ) 
{ 
	_handle = handle; 
}

VOID* IExportStreamReceiver::GetHandle( VOID ) 
{ 
	return _handle; 
}

VOID IExportStreamReceiver::SetHandle( VOID *handle ) 
{ 
	_handle = handle; 
}

HTRecorder::HTRecorder( INotificationReceiver *notifier )
	: _service(NULL)
	, _liveSession(NULL)
	, _UUID(NULL)
	, _notifier(notifier)
{
	_liveSession = Live5_Initialize( MAX_LIVE_CHANNEL, 0 );
}

HTRecorder::~HTRecorder( VOID )
{
	Disconnect();
	if( _service )
	{
		delete _service;
		_service = NULL;
	}

	if( _liveSession )
	{
		//Live5_Destroy( _liveSession );
		_liveSession = NULL;
	}
	if( _UUID ) 
		free( _UUID );
}

BOOL HTRecorder::Connect( WCHAR *UUID, BOOL bRunAsHTRecorder, WCHAR *address, WCHAR *userID, WCHAR *userPW, DWORD dwMaxChannel, DWORD dwNumberOfThread, ULONG fileVersion, ULONG dbVersion )
{
	if( _UUID ) 
		free( _UUID );
	_UUID = _tcsdup( UUID );

	RS_SERVER_INFO_T			serverInfo;
	serverInfo.nId				= ServiceCoordinator::Instance().GetServerID();
	serverInfo.strServerId		= UUID;
	serverInfo.bEnable			= TRUE;
	serverInfo.strName			= _T("IntellVMS HTRecorder Service");
	serverInfo.strAddress		= address;
	serverInfo.nPort			= DEFAULT_NCSERVICE_PORT;
	serverInfo.strUserId		= userID;
	serverInfo.strUserPassword	= userPW;
	
	_service = new ServiceCore( serverInfo.nId, serverInfo.strServerId, _liveSession, this );
	_service->SetAddress( serverInfo.strAddress );
	_service->SetAsyncrousConnection( TRUE );
	_service->SetPort( serverInfo.nPort );
	_service->SetUser( serverInfo.strUserId, serverInfo.strUserPassword );
	if( bRunAsHTRecorder )
		_service->SetProtocol( NAUTILUS_V2_SETUP );
	else
		_service->SetProtocol( NAUTILUS_V2 );

	BOOL bConnected = _service->Connect( serverInfo.strAddress, 
												 serverInfo.nPort, 
												 serverInfo.strUserId, 
												 serverInfo.strUserPassword, 
												 _service->GetProtocol(), 
												 fileVersion, 
												 dbVersion, 
												 _service->IsAsyncrousConnection() );

	INT maxTrialCount = 3;
	if( _service->IsAsyncrousConnection() )
	{
		BOOL IsConnecting=_service->IsConnecting();
		while(IsConnecting && (maxTrialCount>0))
		{
			IsConnecting=_service->IsConnecting();
			bConnected = _service->IsConnected();
			if(bConnected)
				break;

			maxTrialCount--;
			Sleep(1000);
		}
		/*
		while( (_service->IsConnecting()) && (maxTrialCount>0) )
		{
			//APP_PUMP_MESSAGE(NULL, 0, 0);
			maxTrialCount--;
			Sleep(1000);
		}
		bConnected = _service->IsConnected();
		*/
	}

	if( !bConnected ) Disconnect();

	return bConnected;
}

BOOL HTRecorder::Disconnect( VOID )
{
	_service->Disconnect();
	if( _service->IsAsyncrousConnection() )
	{
		while( _service->IsConnected() )
		{
			//APP_PUMP_MESSAGE(NULL, 0, 0);
			Sleep(10);
		}
		Sleep(10);
	}
	return ( !_service->IsConnected() );
}

BOOL HTRecorder::IsConnected( VOID )
{
	if( !_service ) return FALSE;
	else
	{
		return _service->IsConnected();
	}
}

///////////////////////  LOGIN  /////////////////////////
BOOL HTRecorder::KeepAliveRequest( VOID )
{
	if( !_service ) return FALSE;
	return _service->KeepAliveRequest();

}
	
///////////////////////  DEVICE  /////////////////////////
BOOL HTRecorder::GetDeviceList( RS_DEVICE_INFO_SET_T *deviceInfoList )
{
	if( !_service ) return FALSE;
	return _service->GetDeviceList( deviceInfoList );
}

BOOL HTRecorder::CheckDeviceStatus( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_DEVICE_STATUS_SET_T *deviceStatusList )
{
	if( !_service ) return FALSE;
	return _service->CheckDeviceStatus( deviceInfoList, deviceStatusList );
}

BOOL HTRecorder::AddDevice( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_DEVICE_RESULT_STATUS_SET_T *deviceResultStatusList )
{
	if( !_service ) return FALSE;
	return _service->UpdateDevice( deviceInfoList, deviceResultStatusList );
}

BOOL HTRecorder::UpdateDevice( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_DEVICE_RESULT_STATUS_SET_T *deviceResultStatusList )
{
	if( !_service ) return FALSE;
	return _service->UpdateDevice( deviceInfoList, deviceResultStatusList );
}

BOOL HTRecorder::RemoveDevice( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_DEVICE_RESULT_STATUS_SET_T *deviceResultStatusList )
{
	if( !_service ) return FALSE;
	return _service->RemoveDevice( deviceInfoList, deviceResultStatusList );
}

BOOL HTRecorder::RemoveDeviceEx( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_DEVICE_RESULT_STATUS_SET_T *deviceResultStatusList )
{
	if( !_service ) return FALSE;
	return _service->RemoveDeviceEx( deviceInfoList, deviceResultStatusList );
}
	
///////////////////////  RECORDING  /////////////////////////
BOOL HTRecorder::GetRecordingScheduleList( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_RECORD_SCHEDULE_SET_T *recordShcedList )
{
	if( !_service ) return FALSE;
	return _service->GetRecordingScheduleList( deviceInfoList, recordShcedList );
}

BOOL HTRecorder::UpdateRecordingSchedule( RS_RECORD_SCHEDULE_SET_T *recordSchedList, RS_RESPONSE_INFO_SET_T *responseInfoList )
{
	if( !_service ) return FALSE;
	return _service->UpdateRecordingSchedule( recordSchedList, responseInfoList );
}

BOOL HTRecorder::SetRecordingOverwrite( RS_RECORD_OVERWRITE_INFO_T *overwriteInfo, RS_RESPONSE_INFO_T *responseInfo )
{
	if( !_service ) return FALSE;
	return _service->SetRecordingOverwrite( overwriteInfo, responseInfo );
}

BOOL HTRecorder::GetRecordingOverwrite( RS_RECORD_OVERWRITE_INFO_T *overwriteInfo )
{
	if( !_service ) return FALSE;
	return _service->GetRecordingOverwrite( overwriteInfo );
}

BOOL HTRecorder::SetRecordingRetentionTime( RS_RECORD_RETENTION_INFO_T *retentionInfo, RS_RESPONSE_INFO_T *responseInfo )
{
	if( !_service ) return FALSE;
	return _service->SetRecordingRetentionTime( retentionInfo, responseInfo );
}

BOOL HTRecorder::GetRecordingRetentionTime( RS_RECORD_RETENTION_INFO_T *retentionInfo )
{
	if( !_service ) return FALSE;
	return _service->GetRecordingRetentionTime( retentionInfo );
}

BOOL HTRecorder::GetDiskInfo( RS_DISK_INFO_SET_T *diskInfoList )
{
	if( !_service ) return FALSE;
	return _service->GetDiskInfo( diskInfoList );
}

BOOL HTRecorder::ReserveDiskSpace( RS_DISK_INFO_SET_T *diskInfoList, RS_DISK_RESPONSE_SET_T *diskResponseInfoList )
{
	if( !_service ) 
		return FALSE;
	return 
		_service->ReserveDiskSpace( diskInfoList, diskResponseInfoList );
}

BOOL HTRecorder::GetDiskPolicy( RS_DISK_POLICY_SET_T *diskPolicyList )
{
	if( !_service ) return FALSE;
	return _service->GetDiskPolicy( diskPolicyList );
}

BOOL HTRecorder::UpdateDiskPolicy( RS_DISK_INFO_SET_T *diskInfo, RS_DEVICE_INFO_SET_T *deviceInfoList, RS_DISK_RESPONSE_SET_T *diskResponseList )
{
	if( !_service ) return FALSE;
	return _service->UpdateDiskPolicy( diskInfo, deviceInfoList, diskResponseList );
}

BOOL HTRecorder::IsRecording( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_RECORDING_STATUS_SET_T *recordingStatusList )
{
	if( !_service ) return FALSE;
	return _service->IsRecording( deviceInfoList, recordingStatusList );
}

BOOL HTRecorder::StartRecordingRequest( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_RESPONSE_INFO_SET_T *responseInfoList )
{
	if( !_service ) return FALSE;
	return _service->StartRecordingRequest( deviceInfoList, responseInfoList );
}

BOOL HTRecorder::StopRecordingRequest( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_RESPONSE_INFO_SET_T *responseInfoList )
{
	if( !_service ) return FALSE;
	return _service->StopRecordingRequest( deviceInfoList, responseInfoList );
}

BOOL HTRecorder::StartRecordingAll( VOID )
{
	if( !_service ) return FALSE;
	return _service->StartRecordingAll();
}

BOOL HTRecorder::StopRecordingAll( VOID )
{
	if( !_service ) return FALSE;
	return _service->StopRecordingAll();
}

BOOL HTRecorder::StartManualRecording( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_RESPONSE_INFO_SET_T *resposneInfoList )
{
	if( !_service ) return FALSE;
	return _service->StartManualRecording( deviceInfoList, resposneInfoList );
}

BOOL HTRecorder::StopManualRecording( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_RESPONSE_INFO_SET_T *responseInfList )
{
	if( !_service ) return FALSE;
	return _service->StopManualRecording( deviceInfoList, responseInfList );
}

BOOL HTRecorder::DeleteRecordingData( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_DELETE_RESPONSE_SET_T *deleteResponseList )
{
	if( !_service ) return FALSE;
	return _service->DeleteRecordingData( deviceInfoList, deleteResponseList );
}

///////////////// RELAY STREAM
BOOL HTRecorder::GetRelayInfo( VOID *xmlData, RS_RELAY_INFO_T *rlInfo )
{
	if( !_service ) return FALSE;
	return _service->GetRelayInfo( xmlData, rlInfo );
}

BOOL HTRecorder::StartRelay( RS_DEVICE_INFO_T *relayDevice, RS_RELAY_REQUEST_T *rlRequest )
{
	if( !_service ) return FALSE;
	return _service->StartRelay( relayDevice, rlRequest );
}

BOOL HTRecorder::UpdateRelay( RS_DEVICE_INFO_T *relayDevice, RS_RELAY_REQUEST_T *rlRequest )
{
	if( !_service ) return FALSE;
	return _service->UpdateRelay( relayDevice, rlRequest );
}

BOOL HTRecorder::StopRelay( RS_RELAY_INFO_T *rlInfo )
{
	if( !_service ) return FALSE;
	return _service->StopRelay( rlInfo );
}

///////////////// PLAYBACK STREAM
//CALENDAR SEARCH
BOOL HTRecorder::GetYearIndex( RS_PLAYBACK_DEVICE_SET_T *pbDeviceList, RS_SEARCH_REQUEST_T *searchRequest, RS_SEARCH_RESPONSE_SET_T *searchRspList )
{
	if( !_service ) return FALSE;

	//CBitArray *bitArr;   // 1 ~ 12월
	if( !_service->GetYearIndex(pbDeviceList, searchRequest->year/*, &bitArr*/) ) return FALSE;

	searchRspList->validDeviceCount = pbDeviceList->validDeviceCount;
	for( UINT index=0; index<pbDeviceList->validDeviceCount; index++ )
	{
		CBitArray *bitArr = pbDeviceList->deviceInfo[index].GetRecMonth();
		for( UINT index2=0; index2<RS_MAX_MONTH; index2++ )
		{
			if( bitArr->Get(index2) ) searchRspList->responseInfo[index].bMonths[index2] = TRUE;
		}
	}

	return TRUE;
}

BOOL HTRecorder::GetMonthIndex( RS_PLAYBACK_DEVICE_SET_T *pbDeviceList, RS_SEARCH_REQUEST_T *searchRequest, RS_SEARCH_RESPONSE_SET_T *searchRspList )
{
	if( !_service ) return FALSE;
	//CBitArray *bitArr;    // 0 ~ 23시
	if( !_service->GetMonthIndex(pbDeviceList, searchRequest->year, searchRequest->month/*, &bitArr*/) ) return FALSE;

	searchRspList->validDeviceCount = pbDeviceList->validDeviceCount;
	for( UINT index=0; index<pbDeviceList->validDeviceCount; index++ )
	{
		CBitArray *bitArr = pbDeviceList->deviceInfo[index].GetRecDay();
		for( UINT index2=0; index2<RS_MAX_DAY; index2++ )
		{
			if( bitArr->Get(index2) ) searchRspList->responseInfo[index].months[searchRequest->month-1].bDays[index2] = TRUE;
		}
	}
	return TRUE;
}

BOOL HTRecorder::GetDayIndex( RS_PLAYBACK_DEVICE_SET_T *pbDeviceList, RS_SEARCH_REQUEST_T *searchRequest, RS_SEARCH_RESPONSE_SET_T *searchRspList )
{
	if( !_service ) return FALSE;

	//CBitArray *bitArr;    // 0 ~ 23시
	if( !_service->GetDayIndex( pbDeviceList, searchRequest->year, searchRequest->month, searchRequest->day/*, &bitArr*/) ) return FALSE;

	searchRspList->validDeviceCount = pbDeviceList->validDeviceCount;
	for( UINT index=0; index<pbDeviceList->validDeviceCount; index++ )
	{
		CBitArray *bitArr = pbDeviceList->deviceInfo[index].GetRecHour();
		for( UINT index2=0; index2<RS_MAX_HOUR; index2++ )
		{
			if( bitArr->Get(index2) ) 
				searchRspList->responseInfo[index].months[searchRequest->month-1].days[searchRequest->day-1].bHours[index2] = TRUE;
		}
	}
	return TRUE;
}

BOOL HTRecorder::GetHourIndex( RS_PLAYBACK_DEVICE_SET_T *pbDeviceList, RS_SEARCH_REQUEST_T *searchRequest, RS_SEARCH_RESPONSE_SET_T *searchRspList )
{
	if( !_service ) return FALSE;
	
	//CBitArray *bitArr(64);   // 0 ~ 59분
	//CBitArray bitDupArr;
	if( !_service->GetHourIndex(pbDeviceList, searchRequest->year, searchRequest->month, searchRequest->day, searchRequest->hour/*, &bitArr, &bitDupArr*/) ) return FALSE;


	searchRspList->validDeviceCount = pbDeviceList->validDeviceCount;
	for( UINT index=0; index<pbDeviceList->validDeviceCount; index++ )
	{
		CBitArray *bitArr = pbDeviceList->deviceInfo[index].GetRecMin();
		for( UINT index2=0; index2<RS_MAX_MINUTE; index2++ )
		{
			if( bitArr->Get(index2) ) 
			{
				searchRspList->responseInfo[index].bMonths[searchRequest->month-1] = TRUE;
				searchRspList->responseInfo[index].months[searchRequest->month-1].bDays[searchRequest->day-1] = TRUE;
				searchRspList->responseInfo[index].months[searchRequest->month-1].days[searchRequest->day-1].bHours[searchRequest->hour] = TRUE;
				searchRspList->responseInfo[index].months[searchRequest->month-1].days[searchRequest->day-1].hours[searchRequest->hour].bMinutes[index2] = TRUE;
			}
		}
	}
	return TRUE;
}

BOOL HTRecorder::GetMonths( RS_PLAYBACK_DEVICE_SET_T *pbDeviceList, RS_SEARCH_REQUEST_T *searchRequest, CAtlArray<CString> *months )
{
	if( !_service ) return FALSE;

	if( !_service->GetYearIndex(pbDeviceList, searchRequest->year) ) return FALSE;
	for( UINT index=0; index<pbDeviceList->validDeviceCount; index++ )
	{
		CBitArray *bitArr = pbDeviceList->deviceInfo[index].GetRecMonth();
		for( UINT index2=0; index2<RS_MAX_MONTH; index2++ )
		{
			if( bitArr->Get(index2) )
			{
				CString month;
				month.Format(_T("%d"), index2+1 );
				months->Add( month );
			}
		}
	}
	return TRUE;
}

BOOL HTRecorder::GetDays( RS_PLAYBACK_DEVICE_SET_T *pbDeviceList, RS_SEARCH_REQUEST_T *searchRequest, CAtlArray<CString> *days )
{
	if( !_service ) return FALSE;

	if( !_service->GetMonthIndex(pbDeviceList, searchRequest->year, searchRequest->month) ) return FALSE;
	for( UINT index=0; index<pbDeviceList->validDeviceCount; index++ )
	{
		CBitArray *bitArr = pbDeviceList->deviceInfo[index].GetRecDay();
		for( UINT index2=0; index2<RS_MAX_DAY; index2++ )
		{
			if( bitArr->Get(index2) )
			{
				CString day;
				day.Format(_T("%d"), index2+1 );
				days->Add( day );
			}
		}
	}
	return TRUE;
}

BOOL HTRecorder::GetHours( RS_PLAYBACK_DEVICE_SET_T *pbDeviceList, RS_SEARCH_REQUEST_T *searchRequest, CAtlArray<CString> *hours )
{
	if( !_service ) return FALSE;

	if( !_service->GetDayIndex(pbDeviceList, searchRequest->year, searchRequest->month, searchRequest->day) ) return FALSE;
	for( UINT index=0; index<pbDeviceList->validDeviceCount; index++ )
	{
		CBitArray *bitArr = pbDeviceList->deviceInfo[index].GetRecHour();
		for( UINT index2=0; index2<RS_MAX_HOUR; index2++ )
		{
			if( bitArr->Get(index2) )
			{
				CString hour;
				hour.Format(_T("%d"), index2 );
				hours->Add( hour );
			}
		}
	}
	return TRUE;
}

BOOL HTRecorder::GetMinutes( RS_PLAYBACK_DEVICE_SET_T *pbDeviceList, RS_SEARCH_REQUEST_T *searchRequest, CAtlArray<CString> *minutes )
{
	if( !_service ) return FALSE;

	if( !_service->GetHourIndex(pbDeviceList, searchRequest->year, searchRequest->month, searchRequest->day, searchRequest->hour) ) return FALSE;
	for( UINT index=0; index<pbDeviceList->validDeviceCount; index++ )
	{
		CBitArray *bitArr = pbDeviceList->deviceInfo[index].GetRecMin();
		for( UINT index2=0; index2<RS_MAX_MINUTE; index2++ )
		{
			if( bitArr->Get(index2) )
			{
				CString minute;
				minute.Format(_T("%d"), index2 );
				minutes->Add( minute );
			}
		}
	}
	return TRUE;
}

//PLAYBACK
BOOL HTRecorder::StartPlayback( RS_PLAYBACK_DEVICE_SET_T *pbDeviceList, RS_PLAYBACK_REQUEST_T *pbRequest, RS_PLAYBACK_INFO_T *pbInfo )
{
	if( !_service ) return FALSE;
	return _service->StartPlayback( pbDeviceList, pbRequest, pbInfo );
}

BOOL HTRecorder::GetPlaybackInfo( VOID *xmlData, RS_PLAYBACK_INFO_T *pbInfo )
{
	if( !_service ) return FALSE;
	return _service->GetPlaybackInfo( xmlData, pbInfo );
}

BOOL HTRecorder::StopPlayback( RS_PLAYBACK_INFO_T *pbInfo )
{
	if( !_service ) return FALSE;
	return _service->StopPlayback( pbInfo );
}

//PLAYBACK CONTROL
BOOL HTRecorder::ControlPlay( RS_PLAYBACK_INFO_T *pbInfo )
{
	if( !_service ) return FALSE;
	pbInfo->bOnlyKeyFrame = FALSE;
	return _service->ControlPlay( pbInfo );
}

BOOL HTRecorder::ControlFowardPlay( RS_PLAYBACK_INFO_T *pbInfo )
{
	if( !_service ) return FALSE;
	return _service->ControlFowardPlay( pbInfo );
}

BOOL HTRecorder::ControlBackwardPlay( RS_PLAYBACK_INFO_T *pbInfo )
{
	if( !_service ) return FALSE;
	return _service->ControlBackwardPlay( pbInfo );
}

BOOL HTRecorder::ControlStop( RS_PLAYBACK_INFO_T *pbInfo )
{
	if( !_service ) return FALSE;
	return _service->ControlStop( pbInfo );
}

BOOL HTRecorder::ControlPause( RS_PLAYBACK_INFO_T *pbInfo )
{
	if( !_service ) return FALSE;
	return _service->ControlPause( pbInfo );
}

BOOL HTRecorder::ControlResume( RS_PLAYBACK_INFO_T *pbInfo )
{
	if( !_service ) return FALSE;
	return _service->ControlResume( pbInfo );
}

BOOL HTRecorder::ControlJump( RS_PLAYBACK_JUMP_REQUEST_T *pbRequest, RS_PLAYBACK_INFO_T *pbInfo )
{
	if( !_service ) return FALSE;
	return _service->ControlJump( pbRequest, pbInfo );
}

BOOL HTRecorder::ControlGoToFirst( RS_PLAYBACK_INFO_T *pbInfo, RS_PLAYBACK_GOTO_FIRST_RESPONSE_T *pbResponse )
{
	if( !_service ) return FALSE;
	return _service->ControlGoToFirst( pbInfo, pbResponse );
}

BOOL HTRecorder::ControlGoToLast( RS_PLAYBACK_INFO_T *pbInfo, RS_PLAYBACK_GOTO_LAST_RESPONSE_T *pbResponse )
{
	if( !_service ) return FALSE;
	return _service->ControlGoToLast( pbInfo, pbResponse );
}

BOOL HTRecorder::ControlForwardStep( RS_PLAYBACK_INFO_T *pbInfo )
{
	if( !_service ) return FALSE;
	return _service->ControlForwardStep( pbInfo );
}

BOOL HTRecorder::ControlBackwardStep( RS_PLAYBACK_INFO_T *pbInfo )
{
	if( !_service ) return FALSE;
	return _service->ControlBackwardStep( pbInfo );
}

///////////////// Export
BOOL HTRecorder::StartExport( RS_DEVICE_INFO_SET_T *devInfoList, RS_EXPORT_REQUEST_T *expRequest, RS_EXPORT_RESPONSE_T *expResponse )
{
	if( !_service ) return FALSE;
	return _service->StartExport( devInfoList, expRequest, expResponse );
}

BOOL HTRecorder::StopExport( RS_EXPORT_INFO_T *expInfo )
{
	if( !_service ) return FALSE;
	return _service->StopExport( expInfo );
}

BOOL HTRecorder::PauseExport( RS_EXPORT_INFO_T *expInfo )
{
	if( !_service ) return FALSE;
	return _service->PauseExport( expInfo );
}

BOOL HTRecorder::ResumeExport( RS_EXPORT_INFO_T *expInfo )
{
	if( !_service ) return FALSE;
	return _service->ResumeExport( expInfo );
}

///////////////// ServerNotification
VOID HTRecorder::OnConnectionStop( RS_CONNECTION_STOP_NOTIFICATION_T *notification )
{
	if( _notifier )
	{
		notification->strUUID = _UUID;
		_notifier->OnConnectionStop( notification );
	}
}

VOID HTRecorder::OnRecordingStorageFull( RS_STORAGE_FULL_NOTIFICATION_T *notification )
{
	if( _notifier )
	{
		notification->strUUID = _UUID;
		_notifier->OnRecordingStorageFull( notification );
	}
}

VOID HTRecorder::OnReservedStorageFull( RS_STORAGE_FULL_NOTIFICATION_T *notification )
{
	if( _notifier )
	{
		notification->strUUID = _UUID;
		_notifier->OnReservedStorageFull( notification );
	}
}

VOID HTRecorder::OnOverwritingError( RS_OVERWRITE_ERROR_NOTIFICATION_T *notification )
{
	if( _notifier )
	{
		notification->strUUID = _UUID;
		_notifier->OnOverwritingError( notification );
	}
}

VOID HTRecorder::OnConfigurationChanged( RS_CONFIGURATION_CHANGED_NOTIFICATION_T *notification )
{
	if( _notifier )
	{
		notification->strUUID = _UUID;
		_notifier->OnConfigurationChanged( notification );
	}
}

VOID HTRecorder::OnPlaybackError( RS_PLAYBACK_ERROR_NOTIFICATION_T *notification )
{
	if( _notifier )
	{
		notification->strUUID = _UUID;
		_notifier->OnPlaybackError( notification );
	}
}

VOID HTRecorder::OnDiskError( RS_DISK_ERROR_NOTIFICATION_T *notification )
{
	if( _notifier )
	{
		notification->strUUID = _UUID;
		_notifier->OnDiskError( notification );
	}
}

VOID HTRecorder::OnKeyFrameMode( RS_KEY_FRAME_MODE_NOTIFICATION_T *notification )
{
	if( _notifier )
	{
		notification->strUUID = _UUID;
		_notifier->OnKeyFrameMode( notification );
	}
}

VOID HTRecorder::OnBufferClean( RS_BUFFER_CLEAN_NOTIFICATION_T *notification )
{
	if( _notifier )
	{
		notification->strUUID = _UUID;
		_notifier->OnBufferClean( notification );
	}
}