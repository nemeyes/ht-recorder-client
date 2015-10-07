#include "stdafx.h"
#include "HTRecorderIF.h"
#include "HTPlayBackStreamReceiver.h"
#include "HTRelayStreamReceiver.h"
#include "HTNotificationReceiver.h"
#include "XSleep.h"
#include "ScopedLock.h"

UINT lastDayOfMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

HTRecorderIF::HTRecorderIF(BOOL bRunAsRecorder)
	: m_bRunAsRecorder(bRunAsRecorder)
{
	//m_notifier = new HTNotificationReceiver();
}

HTRecorderIF::~HTRecorderIF(VOID)
{
	KillPlayBackStream();
	KillRelayStream();

	{
		CScopedLock lock( &m_lockRecorder );
		std::map<CString, HTRecorder*>::iterator iter2;
		for (iter2 = m_mapRecorderList.begin(); iter2 != m_mapRecorderList.end(); iter2++)
		{
			if( (iter2->second)->IsConnected() )
			{
				(iter2->second)->Disconnect();
			}
			delete iter2->second;
		}
		m_mapRecorderList.clear();
	}

	//delete m_notifier;

	
	/*if(m_pRecTime)
	{
		CScopedLock lock( &m_lockTimeline );
		while ( m_pRecTime->GetSize() > 0 ) 
		{
			HTRECORD_TIME_INFO * pRecTime = (_RECORD_TIME_INFO*)m_pRecTime->GetAt(0);
			if(pRecTime)
			{
				delete pRecTime;
				pRecTime = NULL;
			}
			m_pRecTime->RemoveAt(0);
		}
		delete m_pRecTime; 
		m_pRecTime = NULL;
	}*/
}

HTRecorder * HTRecorderIF::GetRecorder(CString strRecorderUuid, CString strRecorderAddress, CString strRecorderUsername, CString strRecorderPassword, UINT nRetryCount)
{
	RS_SERVER_INFO_T rsServerInfo;
	rsServerInfo.strServerId = strRecorderUuid;
	rsServerInfo.strAddress = strRecorderAddress;
	rsServerInfo.strUserId = strRecorderUsername;
	rsServerInfo.strUserPassword = strRecorderPassword;
	return GetRecorder(&rsServerInfo, nRetryCount);
}

BOOL HTRecorderIF::IsRecording(CString strRecorderUuid, CString strRecorderAddress, CString strRecorderUsername, CString strRecorderPassword, CString strCameraUuid)
{
	HTRecorder * recorder = GetRecorder(strRecorderUuid, strRecorderAddress, strRecorderUsername, strRecorderPassword);
	if (!recorder) 
		return FALSE;

	RS_RECORDING_STATUS_SET_T rsRecordingStatusList;

	RS_DEVICE_INFO_SET_T * reDevInfoList = new RS_DEVICE_INFO_SET_T;
	if (!MakeDeviceInfo(strCameraUuid, &reDevInfoList->deviceInfo[0]))
	{
		delete reDevInfoList;
		return FALSE;
	}
	reDevInfoList->validDeviceCount = 1;
	recorder->IsRecording(reDevInfoList, &rsRecordingStatusList);
	delete reDevInfoList;

	return rsRecordingStatusList.recordingStatus[0].isRecording;
}

BOOL HTRecorderIF::StartRelay(CString strRecorderUuid, CString strRecorderAddress, CString strRecorderUsername, CString strRecorderPassword, CString strCameraUuid, CDisplayLib * pVideoView, unsigned char * key, size_t nChannel)
{
	HTRecorder * recorder = GetRecorder(strRecorderUuid, strRecorderAddress, strRecorderUsername, strRecorderPassword);
	if (!recorder) 
		return FALSE;

	BOOL value = TRUE;

	RS_DEVICE_INFO_T	rlDevice;
	RS_RELAY_REQUEST_T	rlRequest;
	if (!MakeDeviceInfo(strCameraUuid, &rlDevice)) 
		return FALSE;

	rlRequest.pbFrameType = RS_RL_FRAME_ALL;
	HTRelayStreamReceiver * rlStreamReceiver = new HTRelayStreamReceiver(recorder, strCameraUuid, pVideoView, key, nChannel);
	rlRequest.pReceiver = rlStreamReceiver;
	rlStreamReceiver->Start();
	value = recorder->StartRelay(&rlDevice, &rlRequest);
	if (value)
	{
		CScopedLock lock(&m_lockRelayReceiver);
		std::map<CString, RLStreamReceiverList>::iterator rlStreamIter;
		rlStreamIter = m_relayUUID.find(strCameraUuid);
		if (rlStreamIter == m_relayUUID.end())
		{
			RLStreamReceiverList rlStreamReceiverList;
			rlStreamReceiverList.insert(std::make_pair(strCameraUuid, static_cast<IStreamReceiver5*>(rlRequest.pReceiver)));
			m_relayUUID.insert(std::make_pair(strRecorderUuid, rlStreamReceiverList));
		}
		else
		{

			RLStreamReceiverList rlStreamReceiverList = rlStreamIter->second;
			std::pair<std::multimap<CString, IStreamReceiver5*>::iterator, std::multimap<CString, IStreamReceiver5*>::iterator> rlStreamIterPair;
			rlStreamIterPair = rlStreamReceiverList.equal_range(strCameraUuid);
			if (rlStreamIterPair.first == rlStreamIterPair.second)
			{
				rlStreamReceiverList.insert(std::make_pair(strCameraUuid, static_cast<IStreamReceiver5*>(rlRequest.pReceiver)));
				rlStreamIter->second = rlStreamReceiverList;
			}
			else
			{
				BOOL bFound = FALSE;
				std::multimap<CString, IStreamReceiver5*>::iterator rlSteamReceiverIter;
				for (rlSteamReceiverIter = rlStreamIterPair.first; rlSteamReceiverIter != rlStreamIterPair.second; rlSteamReceiverIter++)
				{
					HTRelayStreamReceiver * tmpRLStreamReceiver = static_cast<HTRelayStreamReceiver*>(rlSteamReceiverIter->second);
					if (tmpRLStreamReceiver->GetStreamInfo() == rlStreamReceiver->GetStreamInfo())
					{
						tmpRLStreamReceiver->Stop();
						RS_RELAY_INFO_T rlInfo;
						rlInfo.pReceiver = tmpRLStreamReceiver;
						BOOL sValue = tmpRLStreamReceiver->GetRecorder()->StopRelay(&rlInfo);
						do { ::Sleep(10); } while (tmpRLStreamReceiver->IsConnected());
						delete tmpRLStreamReceiver;

						bFound = TRUE;
						break;
					}
				}

				if (bFound) rlStreamReceiverList.erase(rlSteamReceiverIter);
				rlStreamReceiverList.insert(std::make_pair(strCameraUuid, static_cast<IStreamReceiver5*>(rlRequest.pReceiver)));
				rlStreamIter->second = rlStreamReceiverList;
			}
			rlStreamIter->second = rlStreamReceiverList;
		}
	}
	else
	{
		delete (static_cast<HTRelayStreamReceiver*>(rlStreamReceiver));
	}
	return value;
}

BOOL HTRecorderIF::StopRelay(CString strRecorderUuid, CString strRecorderAddress, CString strRecorderUsername, CString strRecorderPassword, CString strCameraUuid)
{
	BOOL value = TRUE;
	RS_DEVICE_INFO_T	rlDevice;

	if (!MakeDeviceInfo(strCameraUuid, &rlDevice))
		return FALSE;
	{
		CScopedLock lock(&m_lockRelayReceiver);

		std::map<CString, RLStreamReceiverList>::iterator iter = m_relayUUID.find(strRecorderUuid);
		if (iter != m_relayUUID.end())
		{
			RLStreamReceiverList rlStreamReceiverList = iter->second;
			std::pair<RLStreamReceiverList::iterator, RLStreamReceiverList::iterator> rlStreamIterPair = rlStreamReceiverList.equal_range(strCameraUuid);
			RLStreamReceiverList::iterator rlStreamIter;
			for (rlStreamIter = rlStreamIterPair.first; rlStreamIter != rlStreamIterPair.second; rlStreamIter++)
			{
				HTRelayStreamReceiver * rlStreamReceiver = static_cast<HTRelayStreamReceiver*>(rlStreamIter->second);
				rlStreamReceiver->Stop();
				RS_RELAY_INFO_T rlInfo;
				rlInfo.pReceiver = rlStreamReceiver;
				value = rlStreamReceiver->GetRecorder()->StopRelay(&rlInfo);
				do 
				{ 
					::Sleep(10); 
				} while (rlStreamReceiver->IsConnected());

				delete rlStreamReceiver;
				rlStreamIter->second = NULL;
			}
			rlStreamReceiverList.erase(strCameraUuid);
			m_relayUUID.erase(strRecorderUuid);
			m_relayUUID.insert(std::make_pair(strRecorderUuid, rlStreamReceiverList));
		}
	}
	return value;
}


//private function
HTRecorder * HTRecorderIF::GetRecorder(RS_SERVER_INFO_T * serverInfo, UINT nRetryCount)
{
	if (serverInfo->strAddress.GetLength() < 1)
		return NULL;

	std::map<CString, HTRecorder*>::iterator iter;
	BOOL bRetry = FALSE;
	if (nRetryCount>0) bRetry = TRUE;

	CScopedLock lock(&m_lockRecorder);
	iter = m_mapRecorderList.find(serverInfo->strServerId);
	if (iter == m_mapRecorderList.end())
	{
		TRACE("connect new list \n");

		HTRecorder * recorder = new HTRecorder(m_notifier);
		BOOL isConnected = recorder->IsConnected();
		if (!isConnected)
		{
			isConnected = recorder->Connect((LPTSTR)(LPCTSTR)serverInfo->strServerId, m_bRunAsRecorder, (LPTSTR)(LPCTSTR)serverInfo->strAddress, (LPTSTR)(LPCTSTR)serverInfo->strUserId, (LPTSTR)(LPCTSTR)serverInfo->strUserPassword);
		}
		if (isConnected)
		{
			TRACE("connect new list success \n");
			m_mapRecorderList.insert(std::make_pair(serverInfo->strServerId, recorder));
			return recorder;
		}
		else
		{
			TRACE("connect new list fail \n");
			m_mapRecorderList.insert(std::make_pair(serverInfo->strServerId, recorder));
			return NULL;
		}
	}
	else
	{
		HTRecorder* recorder = (*iter).second;
		BOOL isConnected = recorder->IsConnected();
		if (!isConnected)
		{
			isConnected = recorder->Connect((LPTSTR)(LPCTSTR)serverInfo->strServerId, m_bRunAsRecorder, (LPTSTR)(LPCTSTR)serverInfo->strAddress, (LPTSTR)(LPCTSTR)serverInfo->strUserId, (LPTSTR)(LPCTSTR)serverInfo->strUserPassword);
			TRACE("connect old list success \n");
		}
		if (isConnected)
			return recorder;
		else
		{
			TRACE("connect old list fail \n");
			return NULL;
		}
	}
}

BOOL HTRecorderIF::MakeDeviceInfo(CString strCameraUuid, RS_DEVICE_INFO_T * rsDeviceInfo)
{
	if (wcslen(strCameraUuid)<1) 
		return FALSE;
	rsDeviceInfo->SetDeviceType(RS_DEVICE_ONVIF_CAMERA);
	rsDeviceInfo->SetID((CString)strCameraUuid);
	return TRUE;
}

VOID HTRecorderIF::KillRelayStream( VOID )
{
	CScopedLock lock( &m_lockRelayReceiver );
	std::map<CString,RLStreamReceiverList>::iterator iter;
	for (iter = m_relayUUID.begin(); iter != m_relayUUID.end(); iter++)
	{
		RLStreamReceiverList rlStreamReceiverList = iter->second;
		if( rlStreamReceiverList.size()<1 ) continue;
		RLStreamReceiverList::iterator iter2;
		for( iter2=rlStreamReceiverList.begin(); iter2!=rlStreamReceiverList.end(); iter2++ )
		{
			HTRelayStreamReceiver *rlStreamReceiver = static_cast<HTRelayStreamReceiver*>(iter2->second);
			rlStreamReceiver->Stop();
			RS_RELAY_INFO_T rlInfo;
			rlInfo.pReceiver = rlStreamReceiver;
			if( rlStreamReceiver->GetRecorder()->StopRelay(&rlInfo) )
			{
				do 
				{ 
					::Sleep( 10 ); 
				} while(rlStreamReceiver->IsConnected());

				delete rlStreamReceiver;
			}
		}
		rlStreamReceiverList.clear();
	}
	m_relayUUID.clear();
}

VOID HTRecorderIF::KillPlayBackStream(VOID)
{
	CScopedLock lock(&m_lockPlaybackReceiver);
	std::map<CString, PBStreamReceiverList>::iterator iter;
	for (iter = m_playbackUUID.begin(); iter != m_playbackUUID.end(); iter++)
	{
		PBStreamReceiverList pbStreamReceiverList = iter->second;
		if (pbStreamReceiverList.size()<1) 
			continue;
		PBStreamReceiverList::iterator iter2;
		for (iter2 = pbStreamReceiverList.begin(); iter2 != pbStreamReceiverList.end(); iter2++)
		{
			HTPlayBackStreamReceiver * pbStreamReceiver = static_cast<HTPlayBackStreamReceiver*>(iter2->second);
			pbStreamReceiver->Stop();
			RS_PLAYBACK_INFO_T pbInfo;
			pbInfo.pReceiver = pbStreamReceiver;
			pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
			if (pbStreamReceiver->GetRecorder()->StopPlayback(&pbInfo))
			{
				int cnt = 0;
				do {
					::Sleep(20);
					cnt++;
					if (cnt > 150) break;
				} while (pbStreamReceiver->IsConnected());
			}
			delete pbStreamReceiver;
			iter2->second = NULL;
		}
		pbStreamReceiverList.clear();
	}
	m_playbackUUID.clear();
}

/*
BOOL HTRecorderIF::StartPlayback( RecStreamRequstInfo *vcamInfo)
{	
	Recorder *recorder = GetRecorder( vcamInfo );
	if( !recorder ) return FALSE;

	BOOL value = FALSE;
	RS_PLAYBACK_DEVICE_SET_T pbDevices = {0,};
	RS_PLAYBACK_REQUEST_T pbRequest;
	RS_PLAYBACK_INFO_T pbInfo = {0,};

	if( !MakeDeviceInfo(vcamInfo, &pbDevices.deviceInfo[0]) ) return FALSE;
	pbDevices.validDeviceCount = 1;
	pbRequest.year	= vcamInfo->playbackTime.wYear;
	pbRequest.month = vcamInfo->playbackTime.wMonth;
	pbRequest.day	= vcamInfo->playbackTime.wDay;
	pbRequest.hour	= vcamInfo->playbackTime.wHour;
	pbRequest.minute= vcamInfo->playbackTime.wMinute;
	pbRequest.second= vcamInfo->playbackTime.wSecond;
	pbRequest.pbDirection = RS_PLAYBACK_DIRECTION_T_FORWARD;
	pbRequest.pbFrameType = RS_PB_FRAME_ALL;

	//if(vcamInfo->speed>=0)
	//	pbRequest.pbDirection = RS_PLAYBACK_DIRECTION_T_FORWARD;
	//else
	//	pbRequest.pbDirection = RS_PLAYBACK_DIRECTION_T_BACKWARD;
	
	TRACE("Playback request Time: %04d %02d %02d %02d %02d %02d\n",pbRequest.year,pbRequest.month,pbRequest.day, pbRequest.hour, pbRequest.minute,pbRequest.second);

	PlayBackStreamReceiver *pbStreamReceiver = new PlayBackStreamReceiver( recorder );
	pbStreamReceiver->SetStreamInfo( pbDevices.deviceInfo[0].GetID());
	pbStreamReceiver->SetStreamClientInfo( vcamInfo->clientUUID );
	UINT32 index2 = 0;
	for( index2=0; index2<pbDevices.validDeviceCount; index2++ )
	{
		pbStreamReceiver->AddStreamInfo( index2, pbDevices.deviceInfo[index2].GetID() );
	}
	pbStreamReceiver->Start();
	pbRequest.pReceiver = pbStreamReceiver;
	value = recorder->StartPlayback( &pbDevices, &pbRequest, &pbInfo );

	if( value )
	{
		//chInfo->m_pCamera->StartPlayBack();/////
		CScopedLock lock( &_lockOfPbReceiver );
		std::map<CString,PBStreamReceiverList>::iterator iter;
		iter = _pbUUID.find( vcamInfo->vcamRcrdUuid );
		if( iter==_pbUUID.end() )
		{
			PBStreamReceiverList pbStreamReceiverList;
			pbStreamReceiverList.insert( std::make_pair(vcamInfo->vcamUuid, static_cast<IStreamReceiver5*>(pbRequest.pReceiver)) );
			_pbUUID.insert( std::make_pair(vcamInfo->vcamRcrdUuid, pbStreamReceiverList) );
		}
		else
		{
			PBStreamReceiverList pbStreamReceiverList = iter->second;
			PBStreamReceiverList::iterator iter2;
			iter2 = pbStreamReceiverList.find( vcamInfo->vcamUuid );
			if( iter2==pbStreamReceiverList.end() )
			{
				pbStreamReceiverList.insert( std::make_pair(vcamInfo->vcamUuid, static_cast<IStreamReceiver5*>(pbRequest.pReceiver)) );
				iter->second = pbStreamReceiverList;
			}
			else
			{
				delete ( static_cast<PlayBackStreamReceiver*>(iter2->second) );
				(iter2->second) = static_cast<IStreamReceiver5*>( pbRequest.pReceiver );
			}
		}
	}
	else
	{
		delete ( static_cast<PlayBackStreamReceiver*>(pbStreamReceiver) );
	}
	return value;
}

BOOL HTRecorderIF::StopPlayback( RecStreamRequstInfo *vcamInfo )
{
	BOOL value = FALSE;
	{
		CScopedLock lock( &_lockOfPbReceiver );

		std::map<CString, PBStreamReceiverList>::iterator iter = _pbUUID.find( vcamInfo->vcamRcrdUuid );
		if( iter!=_pbUUID.end() )
		{
			PBStreamReceiverList pbStreamReceiverList = iter->second;
			PBStreamReceiverList::iterator iter2 = pbStreamReceiverList.find( vcamInfo->vcamUuid );
			if( iter2!=pbStreamReceiverList.end() )
			{
				PlayBackStreamReceiver *pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
				pbStreamReceiver->Stop();
				RS_PLAYBACK_INFO_T pbInfo = {0,};
				pbInfo.pReceiver = pbStreamReceiver;
				pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
				value = pbStreamReceiver->GetRecorder()->StopPlayback( &pbInfo );

				int cnt = 0;
				do { 
					::Sleep( 20 );
					cnt++;
					if(cnt > 150) break;
				} while( pbStreamReceiver->IsConnected() );

				pbStreamReceiver->stopPlaybackStreamer();
				delete pbStreamReceiver;
				iter2->second = NULL;

				pbStreamReceiverList.erase( vcamInfo->vcamUuid );
			}
			_pbUUID.erase( vcamInfo->vcamRcrdUuid );
			_pbUUID.insert( std::make_pair(vcamInfo->vcamRcrdUuid, pbStreamReceiverList) );
		}
	}
	//chInfo->m_pCamera->StopPlayBack();
	return value;
}


BOOL HTRecorderIF::ControlPause( RecStreamRequstInfo *vcamInfo )
{
	BOOL value = FALSE;
	RS_PLAYBACK_INFO_T pbInfo = {0,};

	if( vcamInfo )
	{
		CScopedLock lock( &_lockOfPbReceiver );
		std::map<CString,PBStreamReceiverList>::iterator iter;
		iter = _pbUUID.find( vcamInfo->vcamRcrdUuid );
		if( iter==_pbUUID.end() ) value = FALSE;
		else
		{
			PBStreamReceiverList pbStreamReceiverList = iter->second;
			PBStreamReceiverList::iterator iter2;
			iter2 = pbStreamReceiverList.find( vcamInfo->vcamUuid );
			if( iter2==pbStreamReceiverList.end() ) value = FALSE;
			else
			{
				PlayBackStreamReceiver *pbStreamReceiver;
				pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
				if( pbStreamReceiver )
				{
					Recorder *recorder = GetRecorder( vcamInfo );
					if( !recorder ) return FALSE;

					pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
					value = recorder->ControlPause( &pbInfo );
				}
				else value = FALSE;
			}
		}
	}
	return value;
}

BOOL HTRecorderIF::ControlResume( RecStreamRequstInfo *vcamInfo )
{
	BOOL value = FALSE;
	RS_PLAYBACK_INFO_T pbInfo = {0,};
	if( vcamInfo )
	{
		CScopedLock lock( &_lockOfPbReceiver );
		std::map<CString,PBStreamReceiverList>::iterator iter;
		iter = _pbUUID.find( vcamInfo->vcamRcrdUuid );
		if( iter==_pbUUID.end() ) value = FALSE;
		else
		{
			PBStreamReceiverList pbStreamReceiverList = iter->second;
			PBStreamReceiverList::iterator iter2;
			iter2 = pbStreamReceiverList.find( vcamInfo->vcamUuid );
			if( iter2==pbStreamReceiverList.end() ) value = FALSE;
			else
			{
				PlayBackStreamReceiver *pbStreamReceiver;
				pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
				if( pbStreamReceiver )
				{
					Recorder *recorder = GetRecorder( vcamInfo );
					if( !recorder ) return FALSE;

					pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
					value = recorder->ControlResume( &pbInfo );
				}
				else value = FALSE;
			}
		}
	}
	return value;
}

BOOL HTRecorderIF::JumpPlaybackByInterval(RecStreamRequstInfo *vcamInfo, VideoJumpInfo *jumpInfo)
{
	BOOL value = FALSE;
	RS_PLAYBACK_JUMP_REQUEST_T pbJumpRequest = {0,};
	RS_PLAYBACK_INFO_T pbInfo = {0,};

	if( vcamInfo )
	{
		CScopedLock lock( &_lockOfPbReceiver );
		std::map<CString,PBStreamReceiverList>::iterator iter;
		iter = _pbUUID.find( vcamInfo->vcamRcrdUuid );
		if( iter==_pbUUID.end() ) value = FALSE;
		else
		{
			PBStreamReceiverList pbStreamReceiverList = iter->second;
			PBStreamReceiverList::iterator iter2;
			iter2 = pbStreamReceiverList.find( vcamInfo->vcamUuid );
			if( iter2==pbStreamReceiverList.end() ) value = FALSE;
			else
			{
				PlayBackStreamReceiver *pbStreamReceiver;
				pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
				if( pbStreamReceiver )
				{
					Recorder *recorder = GetRecorder( vcamInfo );
					if( !recorder ) return FALSE;

					pbJumpRequest.year	= jumpInfo->requestTime.wYear;
					pbJumpRequest.month = jumpInfo->requestTime.wMonth;
					pbJumpRequest.day	= jumpInfo->requestTime.wDay;
					pbJumpRequest.hour	= jumpInfo->requestTime.wHour;
					pbJumpRequest.minute= jumpInfo->requestTime.wMinute;
					pbJumpRequest.second= jumpInfo->requestTime.wSecond;

					pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
					value = recorder->ControlJump( &pbJumpRequest, &pbInfo );
				}
				else value = FALSE;
			}
		}
	}
	return value;
}


BOOL HTRecorderIF::ControlPlaySpeed( RecStreamRequstInfo *vcamInfo, VideoSpeedInfo *speedInfo )
{
	BOOL value = FALSE;
	RS_PLAYBACK_INFO_T pbInfo = {0,};

	if( vcamInfo )
	{
		CScopedLock lock( &_lockOfPbReceiver );
		std::map<CString,PBStreamReceiverList>::iterator iter;
		iter = _pbUUID.find( vcamInfo->vcamRcrdUuid );
		if( iter==_pbUUID.end() ) value = FALSE;
		else
		{
			PBStreamReceiverList pbStreamReceiverList = iter->second;
			PBStreamReceiverList::iterator iter2;
			iter2 = pbStreamReceiverList.find( vcamInfo->vcamUuid );
			if( iter2==pbStreamReceiverList.end() ) value = FALSE;
			else
			{
				PlayBackStreamReceiver *pbStreamReceiver;
				pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
				if( pbStreamReceiver )
				{
					Recorder *recorder = GetRecorder( vcamInfo->vcamRcrdUuid );
					if( !recorder ) return FALSE;

					pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();

					int speed=speedInfo->speed;
					if(speed<0)
					{
						speed*=(-1);
						if(speed>5)speed=5;
						pbInfo.pbSpeed = RS_PLAYBACK_SPEED( speed );
						value = recorder->ControlBackwardPlay( &pbInfo );
					}
					else
					{
						if(speed>5)speed=5;
						pbInfo.pbSpeed = RS_PLAYBACK_SPEED( speed );
						value = recorder->ControlFowardPlay( &pbInfo );
					}
				}
				else value = FALSE;
			}
		}
	}
	return value;
}

int HTRecorderIF::StartExport( RecStreamRequstInfo *chInfo, ExportInfo *exportInfo,
//	CString szFilePath, 
//	UINT sYear, UINT sMonth, UINT sDay, UINT sHour, UINT sMinute, 
//	UINT eYear, UINT eMonth, UINT eDay, UINT eHour, UINT eMinute, 
	LPEXPORTMSGHANDLER pHandler, LPVOID pParam)
{
	Recorder *recorder	= GetRecorder( chInfo );
	if( !recorder ) return VMS_RESULT_NO_RECORDER;
	//	BOOL value = RESULT_DEFAULT;

	int result = VMS_RESULT_DEFAULT;

	if( chInfo )
	{
		CScopedLock lock( &_lockOfExpReceiver );

		RS_DEVICE_INFO_SET_T *devInfoList = new RS_DEVICE_INFO_SET_T;
		RS_EXPORT_REQUEST_T expReq;
		RS_EXPORT_RESPONSE_T expRes;
		devInfoList->validDeviceCount = 1;
		if( !MakeDeviceInfo(chInfo, &(devInfoList->deviceInfo[0])) ) 
		{
			if( devInfoList ) delete devInfoList;
			return VMS_RESULT_DEFAULT;
		}

		ExportStreamReceiver *exportReceiver = new ExportStreamReceiver();
		exportReceiver->RegisterMsgHandler(pHandler, pParam);

		CTime sTime( exportInfo->startTime.wYear, exportInfo->startTime.wMonth, exportInfo->startTime.wDay, exportInfo->startTime.wHour, exportInfo->startTime.wMinute, 0 );
		CTime eTime( exportInfo->endTime.wYear, exportInfo->endTime.wMonth, exportInfo->endTime.wDay, exportInfo->endTime.wHour, exportInfo->endTime.wMinute, 0 );
		CString strPath;
		strPath.Format(TEXT("%s\\F%04d%02d%02d%02d%02d_%04d%02d%02d%02d%02d.avi"), exportInfo->path, 
			sTime.GetYear(), sTime.GetMonth(), sTime.GetDay(), sTime.GetHour(), sTime.GetMinute(),
			eTime.GetYear(), eTime.GetMonth(), eTime.GetDay(), eTime.GetHour(), eTime.GetMinute() );

		result = exportReceiver->Start( sTime, eTime, strPath );
		if(result != 0)
		{
			if(result==-1 || result==-2)
			{
				result=VMS_RESULT_DEFAULT;
			}
			else 
			{
				result=VMS_RESULT_FAIL_FILE;
			}
			if( devInfoList ) delete devInfoList;
			delete exportReceiver;
			return result;
		}

		expReq.sYear =	 exportInfo->startTime.wYear;
		expReq.sMonth =  exportInfo->startTime.wMonth;
		expReq.sDay =	 exportInfo->startTime.wDay;
		expReq.sHour =	 exportInfo->startTime.wHour;
		expReq.sMinute = exportInfo->startTime.wMinute;
		expReq.sSecond = 0;
		expReq.eYear =	 exportInfo->endTime.wYear;
		expReq.eMonth =	 exportInfo->endTime.wMonth;
		expReq.eDay =	 exportInfo->endTime.wDay;
		expReq.eHour =	 exportInfo->endTime.wHour;
		expReq.eMinute = exportInfo->endTime.wMinute;
		expReq.eSecond = 0;
		expReq.pReceiver = exportReceiver;
		expReq.type = RS_EP_FRAME_ALL;//RS_EP_FRAME_ALL;//RS_EP_FRAME_VIDEO;
		result = recorder->StartExport( devInfoList, &expReq, &expRes );
		if(result)
		{
			m_exportList.insert(pair<RecStreamRequstInfo*,ExportStreamReceiver*>(chInfo,exportReceiver));
			result = VMS_RESULT_SUCCESS;
		}
		else
		{
			result = VMS_RESULT_FAIL_CONNECT;
		}
		if( devInfoList ) delete devInfoList;
	}
	return result;
}

BOOL HTRecorderIF::StopExport( RecStreamRequstInfo *chInfo )
{
	//need to check
	BOOL value = FALSE;
	RS_EXPORT_INFO_T epInfo = {0,};
	if( chInfo )
	{
		CScopedLock lock( &_lockOfExpReceiver );
		map<RecStreamRequstInfo*,ExportStreamReceiver *>::iterator itor;
		itor = m_exportList.find(chInfo);
		if( itor!=m_exportList.end()) 
		{
			itor->second->Stop();
			epInfo.exportID = itor->second->GetExportID();
			Recorder *recorder	= GetRecorder( chInfo );
			value = recorder->StopExport( &epInfo );
			delete itor->second;
			itor->second = NULL;
		}
	}
	return value;
}

void HTRecorderIF::ResetExportList()
{
	std::map<RecStreamRequstInfo*,ExportStreamReceiver*>::iterator iter;
	for(iter=m_exportList.begin();iter!=m_exportList.end();iter++) 
	{
		if(iter->second)
		{
			iter->second;
			delete iter->second;
			iter->second = NULL;
		}
	}
	m_exportList.clear();
}


BOOL HTRecorderIF::GetSelectedTimeline( RecStreamRequstInfo *chInfo, time_t startTime, time_t endTime)
{
	Recorder *recorder = GetRecorder( chInfo );
	if( !recorder ) return FALSE;

	RS_PLAYBACK_DEVICE_SET_T	pbDeviceList;
	if( !MakeDeviceInfo(chInfo, &pbDeviceList.deviceInfo[0]) ) return FALSE;

	RS_SEARCH_RESPONSE_SET_T	*searchRspList = new RS_SEARCH_RESPONSE_SET_T();
	RS_SEARCH_REQUEST_T searchRequest;
	pbDeviceList.validDeviceCount = 1;

	struct tm recStartTime;
	struct tm recEndTime;
	struct tm recFinalTime;
	memset(&recStartTime, 0, sizeof(struct tm));
	memset(&recEndTime, 0, sizeof(struct tm));
	memset(&recFinalTime, 0, sizeof(struct tm));

	CTime cStartTime(startTime);
	CTime cEndTime(endTime);

	SYSTEMTIME stFrom;
	memset(&stFrom, 0, sizeof(SYSTEMTIME));
	stFrom.wYear=cStartTime.GetYear();
	stFrom.wMonth=cStartTime.GetMonth();
	stFrom.wDay=cStartTime.GetDay();

	CTime oConvert(stFrom);
	time_t ttStart=oConvert.GetTime();

	int diffday = (endTime - ttStart)/(60*60*24);	//timespan.GetDays();
	int diffhour =  (endTime - startTime)/(60*60);	//diffday*24 + timespan.GetHours();

	int start_min;
	int end_min;
	int start_hour;
	int end_hour;
	int cnt_add_rec = 0;
	CTime cSearchTime;

	if(diffday == 0)
	{
		if(cStartTime.GetDay() != cEndTime.GetDay()) 
			diffday = 1;
	}

	for(int dIndex=0;dIndex<=diffday;dIndex++)
	{
		cSearchTime = cStartTime + CTimeSpan(dIndex,0,0,0);
		searchRequest.year 	= cSearchTime.GetYear(); 
		searchRequest.month	= cSearchTime.GetMonth(); 
		searchRequest.day	= cSearchTime.GetDay();

		if(recorder->GetDayIndex(&pbDeviceList, &searchRequest, searchRspList))
		{
			start_hour = (dIndex==0) ? cStartTime.GetHour():0;
			end_hour = (dIndex==diffday) ? cEndTime.GetHour():RS_MAX_HOUR-1;

			for(int hIndex=start_hour;hIndex<=end_hour;hIndex++)
			{
				searchRequest.hour=hIndex;
				if(recorder->GetHourIndex(&pbDeviceList, &searchRequest, searchRspList))
				{
					if( searchRspList->responseInfo[0].months[searchRequest.month-1].days[searchRequest.day-1].bHours[searchRequest.hour])
					{
						start_min = (hIndex==start_hour && dIndex==0) ? cStartTime.GetMinute():0;
						end_min = (hIndex==end_hour && dIndex==diffday) ? cEndTime.GetMinute():RS_MAX_MINUTE;

						for(int mIndex=start_min; mIndex<end_min; mIndex++ )
						{
							if( searchRspList->responseInfo[0].months[searchRequest.month-1].days[searchRequest.day-1].hours[searchRequest.hour].bMinutes[mIndex] )
							{
								if( recStartTime.tm_year==0 )
								{
									recStartTime.tm_year = searchRequest.year-1900;
									recStartTime.tm_mon = searchRequest.month-1;
									recStartTime.tm_mday = searchRequest.day;
									recStartTime.tm_hour = searchRequest.hour;
									recStartTime.tm_min = mIndex;
								}

								recFinalTime.tm_year = searchRequest.year-1900;
								recFinalTime.tm_mon = searchRequest.month-1;
								recFinalTime.tm_mday = searchRequest.day;
								recFinalTime.tm_hour = searchRequest.hour;
								recFinalTime.tm_min = mIndex;
							}
							else 
							{
								if( recStartTime.tm_year!=0 )
								{
									AddRecTime(&recStartTime,&recFinalTime);
									memset(&recStartTime, 0, sizeof(struct tm));
									memset(&recEndTime, 0, sizeof(struct tm));
									cnt_add_rec++;
								}
							}
						}
					}
				}
				else
				{
					return FALSE;
				}
			}
		}
		else
		{
			return FALSE;
		}
	}

	if( recStartTime.tm_year!=0 && recEndTime.tm_year == 0)
	{
		AddRecTime(&recStartTime,&recFinalTime);
		memset(&recStartTime, 0, sizeof(struct tm));
		memset(&recEndTime, 0, sizeof(struct tm));
		cnt_add_rec++;
	}

	delete searchRspList;
	if(cnt_add_rec == 0) 
		return FALSE;

	return TRUE;
}

void HTRecorderIF::AddRecTime(tm *startTime,tm *endTime)
{
	if(m_pRecTime)
	{
		CTime curTime=CTime::GetCurrentTime();
		time_t curTime2 = curTime.GetTime();

		time_t t1, t2;
		startTime->tm_isdst=-1;
		t1=mktime(startTime);
		endTime->tm_isdst=-1;
		t2=mktime(endTime);

		if(t1>= t2)
			return;
		if((t1 >= curTime2) || (t2 >= curTime2))
			return;

		_RECORD_TIME_INFO *recTime = new _RECORD_TIME_INFO;
		recTime->startTime = t1;
		recTime->endTime = t2;
		//TRACE(TEXT("\r\n AddRecTime INIT: s:%d, e:%d \r\n "), recTime->startTime, recTime->endTime);

		CScopedLock lock( &_lockOfTimeline );
		int ristCnt = m_pRecTime->GetCount();
		if(ristCnt > 0)
		{
			_RECORD_TIME_INFO * lastTime;
			lastTime = (_RECORD_TIME_INFO *)m_pRecTime->GetAt(ristCnt-1);
			if((recTime->startTime >= lastTime->startTime) && (recTime->startTime <= lastTime->endTime) && (recTime->endTime >= lastTime->endTime))// need to check
			{
				lastTime->endTime = recTime->endTime;
				delete recTime; recTime = NULL;
			}
			else
			{
				//TRACE(TEXT("\r\n AddRecTime: s:%d, e:%d \r\n "), recTime->startTime, recTime->endTime);
				m_pRecTime->Add(recTime);
			}
		}
		else
		{
			//TRACE(TEXT("\r\n AddRecTime: s:%d, e:%d \r\n "), recTime->startTime, recTime->endTime);
			m_pRecTime->Add(recTime);
		}
	}
}

BOOL HTRecorderIF::GetTimelineTrack( RecStreamRequstInfo *vcamInfo, int type, SYSTEMTIME startTime)
{
	if(type==TRACK_TYPE_RETENTION)
	{
		UINT year, month, week, day;
		year=month=week=day=0;
		BOOL bRetention=FALSE;
		int retentionTime=0;

		RS_SERVER_INFO_T serverInfo;
		serverInfo.strServerId =	vcamInfo->vcamRcrdUuid;
		serverInfo.strAddress =		vcamInfo->vcamRcrdIP;
		serverInfo.strUserId =		vcamInfo->vcamRcrdAccessID;
		serverInfo.strUserPassword= vcamInfo->vcamRcrdAccessPW;

		if(GetRecordingRetentionTime(&serverInfo,&bRetention,&year,&month,&week,&day))
		{
			BYTE * timelineBuffer = NULL;
			int sum = sizeof(TimelineHeader);
			int size = sizeof(RECORD_TIME_INFO);
			timelineBuffer = ( BYTE * )malloc( size + sizeof (TimelineHeader ) );

			//response retention time
			TimelineHeader header;
			header.marker = MARKER;
			_tcscpy_s( header.camUUID, vcamInfo->vcamUuid);
			header.type = TRACK_TYPE_RETENTION; //TRACK_TYPE_RETENTION //retention 시간 이전 트랙은 지운다.

			if(bRetention) 		retentionTime = month*30+day;
			else 				retentionTime = 90;
			if(retentionTime==0)retentionTime = 90;

			CTime curTime=CTime::GetCurrentTime();
			CTime retentionStartTime=curTime - CTimeSpan( retentionTime, 0, 0 ,0 );

			struct tm      imtime;
			imtime.tm_year = retentionStartTime.GetYear()-1900;
			imtime.tm_mon  = retentionStartTime.GetMonth()-1;
			imtime.tm_mday = retentionStartTime.GetDay();
			imtime.tm_hour = retentionStartTime.GetHour();
			imtime.tm_min  = retentionStartTime.GetMinute();
			imtime.tm_sec  = retentionStartTime.GetSecond();
			imtime.tm_isdst=-1;
			time_t requestTime = mktime(&imtime);

			RECORD_TIME_INFO info;
			info.startTime = requestTime;
			info.endTime = 0;
			memcpy( timelineBuffer+sum , &info, sizeof (RECORD_TIME_INFO ));

			COPYDATASTRUCT cp;
			cp.dwData = TIMELINE_DATA;
			cp.cbData = sum+size;
			cp.lpData = timelineBuffer;
			//HWND hWndReceiver = ::FindWindow( NULL, TITLE_UI_ENGINE );
			if(g_UIEngineReceiver)
				::SendMessage( g_UIEngineReceiver, WM_COPYDATA, NULL, (LPARAM) &cp );
			FREE_DATA( timelineBuffer );
		}
	}
	else
	{
		BOOL ret=FALSE;
		CString strTrackPath;
		strTrackPath.Format(L"%s\\TrackFileH", g_AppPath);

		CFileFind dirFind;
		ret=dirFind.FindFile(strTrackPath);
		if(!ret)
			ret = CreateDirectory(strTrackPath, NULL);

		CString strUuid = vcamInfo->vcamUuid;
		strUuid.Replace(_T(":"),_T("-"));
		CString strFilePath;
		strFilePath.Format(L"%s\\%s.dat", strTrackPath, strUuid);
		LoadTimeline(strFilePath);

		UINT year, month, week, day;
		year=month=week=day=0;
		BOOL bRetention=FALSE;
		int retentionTime=0;

		RS_SERVER_INFO_T serverInfo;
		serverInfo.strServerId =	vcamInfo->vcamRcrdUuid;
		serverInfo.strAddress =		vcamInfo->vcamRcrdIP;
		serverInfo.strUserId =		vcamInfo->vcamRcrdAccessID;
		serverInfo.strUserPassword= vcamInfo->vcamRcrdAccessPW;

		if(GetRecordingRetentionTime(&serverInfo,&bRetention,&year,&month,&week,&day))
		{
			if(bRetention) 		retentionTime = month*30+day;
			else 				retentionTime = 90;
			if(retentionTime==0)retentionTime = 90;

			CTime curTime = CTime::GetCurrentTime();
			time_t curTime2 = curTime.GetTime();
			int cnt = m_pRecTime->GetCount();

			CScopedLock lock( &_lockOfTimeline );
			if(cnt > 0)
			{
				if(GetSelectedTimeline(vcamInfo, m_endTime, curTime2))
				{
					DeleteRecTimeArray(curTime2, retentionTime);
					UpdateLastTime();
				}
			}
			else
			{
				if(GetSelectedTimeline(vcamInfo, curTime2-(retentionTime*86400), curTime2))
				{
					UpdateLastTime();
				}
			}
		}
		SaveTimeline(strFilePath);
		SendTimelineData(vcamInfo->vcamUuid, startTime);
	}
	return TRUE;
}

void HTRecorderIF::SendTimelineData(CString VCamUUID, SYSTEMTIME startTime)
{
	{
		BYTE * timelineBuffer = NULL;
		if( m_pRecTime ) //트랙이 있으면
		{
			if(startTime.wYear==0)
			{
				int size = sizeof(RECORD_TIME_INFO)*m_pRecTime->GetSize();
				timelineBuffer = ( BYTE * )malloc( size + sizeof (TimelineHeader ) );
			
				//response retention time
				TimelineHeader header;
				header.marker = MARKER;
				_tcscpy_s( header.camUUID, VCamUUID);
				header.type = TRACK_TYPE_VIDEO; //TRACK_TYPE_RETENTION //retention 시간 이전 트랙은 지운다.
				header.count=m_pRecTime->GetSize();

				if( 1 ) //요청한 시간에 트랙이 모두 있으면
				{
					memcpy( timelineBuffer , &header, sizeof (TimelineHeader ));
					int sum = sizeof(TimelineHeader);
					RECORD_TIME_INFO info;

					for (int i=0; i<m_pRecTime->GetSize(); i++) 
					{
						_RECORD_TIME_INFO* pRecTime = (_RECORD_TIME_INFO*)m_pRecTime->GetAt(i);
						if(pRecTime)
						{
							info.startTime = pRecTime->startTime;
							info.endTime = pRecTime->endTime;

							memcpy( timelineBuffer+sum , &info, sizeof (RECORD_TIME_INFO ));
							sum += sizeof (RECORD_TIME_INFO);
						}
					}

					COPYDATASTRUCT cp;
					cp.dwData = TIMELINE_DATA;
					cp.cbData = sum;
					cp.lpData = timelineBuffer;
					//HWND hWndReceiver = ::FindWindow( NULL, TITLE_UI_ENGINE );
					if(g_UIEngineReceiver)
						::SendMessage( g_UIEngineReceiver, WM_COPYDATA, NULL, (LPARAM) &cp );
				}
			}
			else
			{
				//struct tm      imtime;
				//imtime.tm_year = startTime.wYear-1900;
				//imtime.tm_mon  = startTime.wMonth-1;
				//imtime.tm_mday = startTime.wDay;
				//imtime.tm_hour = startTime.wHour;
				//imtime.tm_min  = startTime.wMinute;
				//imtime.tm_sec  = startTime.wSecond;
				//imtime.tm_isdst=-1;
				//time_t requestTime = mktime(&imtime);

				//int count=0;
				//for (int i=m_pRecTime->GetSize()-1; i>=0; i--) 
				//{
				//	_RECORD_TIME_INFO* pRecTime = (_RECORD_TIME_INFO*)m_pRecTime->GetAt(i);
				//	if(pRecTime)
				//	{
				//		if(requestTime>=pRecTime->startTime && requestTime<=pRecTime->endTime )
				//		{
				//			count++;
				//			break;
				//		}
				//	}
				//}

				//if(count>0)
				//{
				//	int size = sizeof(RECORD_TIME_INFO)*count;
				//	timelineBuffer = ( BYTE * )malloc( size + sizeof (TimelineHeader ) );

				//	//response retention time
				//	TimelineHeader header;
				//	header.marker = MARKER;
				//	_tcscpy_s( header.camUUID, VCamUUID);
				//	header.type = TRACK_TYPE_VIDEO; //TRACK_TYPE_RETENTION //retention 시간 이전 트랙은 지운다.
				//	header.count=count;

				//	memcpy( timelineBuffer , &header, sizeof (TimelineHeader ));
				//	int sum = sizeof(TimelineHeader);
				//	RECORD_TIME_INFO info;

				//	for (int i=m_pRecTime->GetSize()-1; i>=0; i--)
				//	{
				//		_RECORD_TIME_INFO* pRecTime = (_RECORD_TIME_INFO*)m_pRecTime->GetAt(i);
				//		if(pRecTime)
				//		{
				//			info.startTime = pRecTime->startTime;
				//			info.endTime = pRecTime->endTime;

				//			memcpy( timelineBuffer+sum , &info, sizeof (RECORD_TIME_INFO ));
				//			sum += sizeof (RECORD_TIME_INFO);
				//		}
				//	}

				//	COPYDATASTRUCT cp;
				//	cp.dwData = TIMELINE_DATA;
				//	cp.cbData = sum;
				//	cp.lpData = timelineBuffer;
				//	HWND hWndReceiver = ::FindWindow( NULL, TITLE_UI_ENGINE );
				//	::SendMessage( hWndReceiver, WM_COPYDATA, NULL, (LPARAM) &cp );
				//}
				
				int count=1;
				int size = sizeof(RECORD_TIME_INFO)*count;
				timelineBuffer = ( BYTE * )malloc( size + sizeof (TimelineHeader ) );

				//response retention time
				TimelineHeader header;
				header.marker = MARKER;
				_tcscpy_s( header.camUUID, VCamUUID);
				header.type = TRACK_TYPE_VIDEO; //TRACK_TYPE_RETENTION //retention 시간 이전 트랙은 지운다.
				header.count=count;

				memcpy( timelineBuffer , &header, sizeof (TimelineHeader ));
				int sum = sizeof(TimelineHeader);
				RECORD_TIME_INFO info;

				int sizeTrack=m_pRecTime->GetSize();
				if(sizeTrack>0)
				{
					_RECORD_TIME_INFO* pRecTime = (_RECORD_TIME_INFO*)m_pRecTime->GetAt(m_pRecTime->GetSize()-1);
					if(pRecTime)
					{
						info.startTime = pRecTime->startTime;
						info.endTime = pRecTime->endTime;

						memcpy( timelineBuffer+sum , &info, sizeof (RECORD_TIME_INFO ));
						sum += sizeof (RECORD_TIME_INFO);
					}

					COPYDATASTRUCT cp;
					cp.dwData = TIMELINE_DATA;
					cp.cbData = sum;
					cp.lpData = timelineBuffer;
					//HWND hWndReceiver = ::FindWindow( NULL, TITLE_UI_ENGINE );
					if(g_UIEngineReceiver)
						::SendMessage( g_UIEngineReceiver, WM_COPYDATA, NULL, (LPARAM) &cp );
				}
			}
		}
		FREE_DATA( timelineBuffer );
	}
}

BOOL HTRecorderIF::GetRecordingRetentionTime( RS_SERVER_INFO_T *serverInfo, BOOL *enable, UINT *year, UINT *month, UINT *week, UINT *day )
{
	Recorder *recorder = GetRecorder( serverInfo );
	if( !recorder ) return FALSE;

	RS_RECORD_RETENTION_INFO_T retentionInfo;
	if( recorder->GetRecordingRetentionTime(&retentionInfo) )
	{
		(*enable) = retentionInfo.enable;
		(*year) = retentionInfo.year;
		(*month) = retentionInfo.month;
		(*week) = retentionInfo.week;
		(*day) = retentionInfo.day;
		return TRUE;
	}
	else return FALSE;
}

BOOL HTRecorderIF::LoadTimeline(CString path)
{
	CScopedLock lock( &_lockOfTimeline );

	if(m_pRecTime != NULL)
		DeleteRecTimeArray();
	m_pRecTime = new CPtrArray;

	CStdioFile file;
	m_endTime = 0;

	BOOL result = file.Open(path, CFile::modeRead);
	if(result)
	{
		CString data;
		while(file.ReadString(data))
		{
			if(data.Compare(TEXT(""))!=0)
			{
				_RECORD_TIME_INFO * time = new _RECORD_TIME_INFO;
				swscanf_s(data,L"%I64u,%I64u",&time->startTime,&time->endTime);
				if((time->startTime > 0) && (time->endTime > 0))
				{
					m_pRecTime->Add(time);
					m_endTime = time->endTime;
				}
				else
				{
					delete time;
				}
			}
		}
		file.Close();
	}

	return TRUE;
}

void HTRecorderIF::DeleteRecTimeArray()
{
	if(m_pRecTime)
	{
		CScopedLock lock( &_lockOfTimeline );
		while ( m_pRecTime->GetSize() > 0 ) 
		{
			_RECORD_TIME_INFO* pRecTime = (_RECORD_TIME_INFO*)m_pRecTime->GetAt(0);
			if(pRecTime)
			{
				delete pRecTime;
				pRecTime = NULL;
			}
			m_pRecTime->RemoveAt(0);
		}
		delete m_pRecTime; 
		m_pRecTime = NULL;
	}
}

void HTRecorderIF::DeleteRecTimeArray(CTime curtime,int retentionTime)
{
	if(m_pRecTime)
	{
		CScopedLock lock( &_lockOfTimeline );
		CTime retentionDay = curtime - CTimeSpan( (retentionTime-1), 0, 0, 0 );
		time_t retiontion_t = retentionDay.GetTime();

		while ( m_pRecTime->GetSize() > 0 ) 
		{
			_RECORD_TIME_INFO* pRecTime = (_RECORD_TIME_INFO*)m_pRecTime->GetAt(0);
			if(pRecTime)
			{
				if(pRecTime->startTime >= retiontion_t) break;
				if(pRecTime->endTime <= retiontion_t)
				{
					delete pRecTime;
					pRecTime = NULL;
					m_pRecTime->RemoveAt(0);
				}
				else
				{
					pRecTime->startTime = retiontion_t;
				}
			}
		}
	}
}

void HTRecorderIF::UpdateLastTime()
{
	CScopedLock lock( &_lockOfTimeline );
	int ristCnt = m_pRecTime->GetCount();
	if(ristCnt > 0)
	{
		_RECORD_TIME_INFO * Time;
		Time = (_RECORD_TIME_INFO *)m_pRecTime->GetAt(ristCnt-1);
		{
			m_endTime = Time->endTime;
		}
	}
}

BOOL HTRecorderIF::SaveTimeline(CString path)
{
	CStdioFile file;
	BOOL result = file.Open(path, CFile::modeCreate | CFile::modeWrite);
	if(result)
	{
		file.SeekToBegin();
		if(m_pRecTime)
		{
			CScopedLock lock( &_lockOfTimeline );
			for(int i=0;i<m_pRecTime->GetCount();i++)
			{
				_RECORD_TIME_INFO * time = (_RECORD_TIME_INFO *)m_pRecTime->GetAt(i);
				CString data;
				data.Format(L"%I64u,%I64u\n",time->startTime,time->endTime);
				file.WriteString(data);
			}
		}
		file.Close();
	}
	return TRUE;
}

BOOL HTRecorderIF::GetRecordingStatus(RecStreamRequstInfo *vcamInfo)
{
	//BOOL flagIsRec=FALSE;
	//if(IsRecording(vcamInfo))
	//	vcamInfo->playbackSpeed=1;
	//else
	//	vcamInfo->playbackSpeed=0;
	//
	//SendMessage(vcamInfo, RESPONSE_RECORDING_STATUS);
	return TRUE;
}
*/
