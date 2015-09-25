#include "stdafx.h"
#include "RecorderIF.h"
#include "ScopedLock.h"
#include "PlayBackStreamReceiver.h"
#include "RelayStreamReceiver.h"
#include "NotificationReceiver.h"
#include "XSleep.h"

//#if defined(DEBUG)
//#pragma comment(lib, "RecorderD.lib")
//#else
//#pragma comment(lib, "Recorder.lib")
//#endif

#ifdef WITH_HITRON_RECORDER

UINT lastDayOfMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

RecorderIF::RecorderIF( BOOL bRunAsRecorder )
	: _bRunAsRecorder(bRunAsRecorder)
	, _exportReceiver(NULL)
{
	::InitializeCriticalSection( &_lockOfPbReceiver );
	::InitializeCriticalSection( &_lockOfRlReceiver );
	::InitializeCriticalSection( &_lockOfRecorder );
	::InitializeCriticalSection( &_lockOfCamera );
	::InitializeCriticalSection( &_lockOfExpReceiver );
	

	_notifier = new NotificationReceiver();
}

RecorderIF::~RecorderIF( VOID )
{
	KillPlayBackStream();
	KillRelayStream();

	{
		CScopedLock lock( &_lockOfRecorder );
		std::map<CString,Recorder*>::iterator iter2;
		for( iter2=_recorderList.begin(); iter2!=_recorderList.end(); iter2++ )
		{
			if( (iter2->second)->IsConnected() )
			{
				(iter2->second)->Disconnect();
			}
			delete iter2->second;
		}
		_recorderList.clear();
	}

	delete _notifier;

	::DeleteCriticalSection( &_lockOfExpReceiver );
	::DeleteCriticalSection( &_lockOfCamera );
	::DeleteCriticalSection( &_lockOfRecorder );
	::DeleteCriticalSection( &_lockOfPbReceiver );
	::DeleteCriticalSection( &_lockOfRlReceiver );
}
/*
BOOL RecorderIF::SetRunAsRecorder( BOOL bRunAsRecorder )
{
	_bRunAsRecorder=bRunAsRecorder;
}*/

/*
BOOL RecorderIF::IsConnected( RS_SERVER_INFO_T *serverInfo )
{
	Recorder * recorder = GetRecorder( serverInfo );
	//if( 
	return recorder->IsConnected();
}


BOOL RecorderIF::Reconnect( RS_SERVER_INFO_T *serverInfo )
{
	Recorder * recorder = GetRecorder( serverInfo );
	if( recorder )
	{
		if( recorder->IsConnected() )
		{
			recorder->Disconnect();
		}
		return  recorder->Connect( (LPTSTR)(LPCTSTR)serverInfo->strServerId, _bRunAsRecorder, (LPTSTR)(LPCTSTR)serverInfo->strAddress, (LPTSTR)(LPCTSTR)serverInfo->strUserId, (LPTSTR)(LPCTSTR)serverInfo->strUserPassword );
	}
	else return FALSE;
}
*/

VOID RecorderIF::KillRelayStream( VOID )
{
	CScopedLock lock( &_lockOfRlReceiver );
	std::map<CString,RLStreamReceiverList>::iterator iter;
	for( iter=_rlUUID.begin(); iter!=_rlUUID.end(); iter++ )
	{
		RLStreamReceiverList rlStreamReceiverList = iter->second;
		if( rlStreamReceiverList.size()<1 ) continue;
		RLStreamReceiverList::iterator iter2;
		for( iter2=rlStreamReceiverList.begin(); iter2!=rlStreamReceiverList.end(); iter2++ )
		{
			RelayStreamReceiver *rlStreamReceiver = static_cast<RelayStreamReceiver*>( iter2->second );
			rlStreamReceiver->Stop();
			RS_RELAY_INFO_T rlInfo;
			rlInfo.pReceiver = rlStreamReceiver;
			if( rlStreamReceiver->GetRecorder()->StopRelay(&rlInfo) )
			{
				do { ::Sleep( 10 ); } while( rlStreamReceiver->IsConnected() );
				delete rlStreamReceiver;
			}
		}
		rlStreamReceiverList.clear();
	}
	_rlUUID.clear();
}

VOID RecorderIF::KillPlayBackStream( VOID )
{
	CScopedLock lock( &_lockOfPbReceiver );
	std::map<CString,PBStreamReceiverList>::iterator iter;
	for( iter=_pbUUID.begin(); iter!=_pbUUID.end(); iter++ )
	{
		PBStreamReceiverList pbStreamReceiverList = iter->second;
		if( pbStreamReceiverList.size()<1 ) continue;
		PBStreamReceiverList::iterator iter2;
		for( iter2=pbStreamReceiverList.begin(); iter2!=pbStreamReceiverList.end(); iter2++ )
		{
			PlayBackStreamReceiver *pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
			pbStreamReceiver->Stop();
			RS_PLAYBACK_INFO_T pbInfo;
			pbInfo.pReceiver = pbStreamReceiver;
			pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
			if( pbStreamReceiver->GetRecorder()->StopPlayback(&pbInfo) )
			{
				do { ::Sleep( 10 ); } while( pbStreamReceiver->IsConnected() );
				delete pbStreamReceiver;
			}
		}
		pbStreamReceiverList.clear();
	}
	_pbUUID.clear();
}

BOOL RecorderIF::GetDeviceList( RS_SERVER_INFO_T *serverInfo, RS_DEVICE_INFO_SET_T *deviceInfoList )
{
	Recorder *recorder = GetRecorder( serverInfo );
	if( !recorder ) return FALSE;
	return recorder->GetDeviceList( deviceInfoList );
}

BOOL RecorderIF::CheckDeviceList( RS_SERVER_INFO_T *serverInfo, CGroupInfo *group )
{
	Recorder *recorder = GetRecorder( serverInfo );
	if( !recorder ) return FALSE;
	RS_DEVICE_INFO_SET_T deviceInfoList;
	BOOL ret = recorder->GetDeviceList( &deviceInfoList );
	if( ret )
	{
		if( deviceInfoList.validDeviceCount==group->GetCount() ) return TRUE;
		else return FALSE;
	}
	else return FALSE;
}

BOOL RecorderIF::AddDevice( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			chInfos.push_back( chInfo );
		}
	}
	return AddDevice( &chInfos );
}

BOOL RecorderIF::AddDevice( std::vector<CChannelInfo*> *chennelInfos )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;
	for( iter=chennelInfos->begin(); iter!=chennelInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = 0;
		CChannelInfo *chInfoClean;
		CChannelInfo *chInfo;
		RS_DEVICE_INFO_SET_T devInfoList;
		RS_DEVICE_RESULT_STATUS_SET_T devRstStatusList;

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;

			chInfoClean = chInfo;
			if( !MakeDeviceInfo(chInfo, &devInfoList.deviceInfo[index]) ) continue;
			size++;
		}
		devInfoList.validDeviceCount = size;
		devRstStatusList.validDeviceCount = size;

		Recorder * recorder = GetRecorder( chInfoClean );
		if( !recorder ) continue;
		sValue = recorder->AddDevice( &devInfoList, &devRstStatusList );
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::AddDevice( CChannelInfo *chInfo )
{
	Recorder * recorder = GetRecorder( chInfo );
	if( !recorder ) return FALSE;

	RS_DEVICE_INFO_SET_T pbDevices;
	RS_DEVICE_RESULT_STATUS_SET_T devRstStatusList;
	pbDevices.validDeviceCount = 1;
	if( !MakeDeviceInfo(chInfo, &pbDevices.deviceInfo[0]) ) return FALSE;
	return recorder->AddDevice( &pbDevices, &devRstStatusList );
}

BOOL RecorderIF::RemoveDevice( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			chInfos.push_back( chInfo );
		}
	}
	return RemoveDevice( &chInfos );
}

BOOL RecorderIF::RemoveDevice( std::vector<CChannelInfo*> *chennelInfos )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;
	for( iter=chennelInfos->begin(); iter!=chennelInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = FALSE;
		CChannelInfo *chInfoClean;
		CChannelInfo *chInfo;
		RS_DEVICE_INFO_SET_T devInfoList;
		RS_DEVICE_RESULT_STATUS_SET_T devRstStatusList;

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;

			chInfoClean = chInfo;
			if( !MakeDeviceInfo(chInfo, &devInfoList.deviceInfo[index]) ) continue;
			size++;
		}
		devInfoList.validDeviceCount = size;
		devRstStatusList.validDeviceCount = size;

		Recorder *recorder = GetRecorder( chInfoClean );
		if( !recorder ) continue;

		sValue = recorder->RemoveDevice( &devInfoList, &devRstStatusList );
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::RemoveDevice( CChannelInfo *chInfo )
{
	Recorder * recorder = GetRecorder( chInfo );
	if( !recorder ) return FALSE;

	RS_DEVICE_INFO_SET_T devInfoList;
	RS_DEVICE_RESULT_STATUS_SET_T devRstStatusList;
	devInfoList.validDeviceCount = 1;
	if( !MakeDeviceInfo(chInfo, &devInfoList.deviceInfo[0]) ) return FALSE;
	return recorder->RemoveDevice( &devInfoList, &devRstStatusList ); 
}

BOOL RecorderIF::RemoveDeviceEx( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			chInfos.push_back( chInfo );
		}
	}
	return RemoveDeviceEx( &chInfos );
}

BOOL RecorderIF::RemoveDeviceEx( std::vector<CChannelInfo*> *chennelInfos )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;
	for( iter=chennelInfos->begin(); iter!=chennelInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = FALSE;
		CChannelInfo *chInfoClean;
		CChannelInfo *chInfo;
		RS_DEVICE_INFO_SET_T devInfoList;
		RS_DEVICE_RESULT_STATUS_SET_T devRstStatusList;

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;

			chInfoClean = chInfo;
			if( !MakeDeviceInfo(chInfo, &devInfoList.deviceInfo[index]) ) continue;
			size++;
		}
		devInfoList.validDeviceCount = size;
		devRstStatusList.validDeviceCount = size;

		Recorder *recorder = GetRecorder( chInfoClean );
		if( !recorder ) continue;
		sValue = recorder->RemoveDeviceEx( &devInfoList, &devRstStatusList );
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::RemoveDeviceEx( CChannelInfo *chInfo )
{
	Recorder * recorder = GetRecorder( chInfo );
	if( !recorder ) return FALSE;

	RS_DEVICE_INFO_SET_T devInfoList;
	RS_DEVICE_RESULT_STATUS_SET_T devRstStatusList;
	devInfoList.validDeviceCount = 1;
	if( !MakeDeviceInfo(chInfo, &devInfoList.deviceInfo[0]) ) return FALSE;

	return recorder->RemoveDeviceEx( &devInfoList, &devRstStatusList ); 
}

BOOL RecorderIF::CheckDeviceStatus( CGroupInfo *group, RS_DEVICE_STATUS_SET_T *deviceStatusList )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}
	return CheckDeviceStatus( &chInfos, deviceStatusList );
}

BOOL RecorderIF::CheckDeviceStatus( std::vector<CChannelInfo*> *chInfos, RS_DEVICE_STATUS_SET_T *deviceStatusList )
{
	BOOL value = TRUE;
	int recodingStatus = 0;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;
	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = FALSE;
		CChannelInfo *chInfoClean;
		CChannelInfo *chInfo;
		RS_DEVICE_INFO_SET_T devInfoList;
		RS_DEVICE_STATUS_SET_T devStatusList;

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;

			chInfoClean = chInfo;
			if( !MakeDeviceInfo(chInfo, &devInfoList.deviceInfo[index])) continue;
			size++;
		}
		devInfoList.validDeviceCount = size;
		devStatusList.validDeviceCount = size;

		Recorder * recorder = GetRecorder( chInfoClean );
		if( !recorder ) continue;
		sValue = recorder->CheckDeviceStatus( &devInfoList, &devStatusList );
		if( sValue )
		{
			UINT validDeviceCount = deviceStatusList->validDeviceCount;
			for( UINT i=0; i<devStatusList.validDeviceCount;i++ )
			{
				deviceStatusList->deviceStatusInfo[validDeviceCount+i] = devStatusList.deviceStatusInfo[i];
				deviceStatusList->validDeviceCount++;
			}
		}
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::CheckDeviceStatus( CChannelInfo *chInfo, RS_DEVICE_STATUS_SET_T *deviceStatusList )
{
	Recorder *recorder = GetRecorder( chInfo );
	if( !recorder ) return FALSE;

	RS_DEVICE_INFO_SET_T deviceInfoList;
	deviceInfoList.validDeviceCount = 1;
	if( !MakeDeviceInfo(chInfo, &deviceInfoList.deviceInfo[0]) ) return FALSE;

	return recorder->CheckDeviceStatus( &deviceInfoList, deviceStatusList );
}

///////////////// RELAY
BOOL RecorderIF::StartRelay( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	CCameraDlg *pCameraDlg;
	CCamera *pCamera;
	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo)
			{
				chInfos.push_back( chInfo );
				pCamera = chInfo->m_pCamera;
				pCameraDlg = g_2DDlgManager->Get2DDlgInfo(index)->m_pCameraDlg;
				pCamera->m_pCameraDlg = pCameraDlg;
				pCameraDlg->m_pCamera = pCamera;
				chInfo->m_pCamera->StartRelay();
			}
		}
	}
	return StartRelay( group->GetGroupUUID(), &chInfos );
}

BOOL RecorderIF::StartRelay( CString groupUUID, std::vector<CChannelInfo*> *chInfos )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = FALSE;
		CChannelInfo *chInfoClean;
		CChannelInfo *chInfo;
		
		RS_DEVICE_INFO_SET_T	rlDevices;
		RS_RELAY_REQUEST_T		rlRequest;

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			 chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;

			chInfoClean = chInfo;

			if( !MakeDeviceInfo(chInfo, &rlDevices.deviceInfo[index]) ) continue;
			size++;
		}
		rlDevices.validDeviceCount = size;

		Recorder *recorder = GetRecorder( chInfoClean );
		if( !recorder ) continue;

		UINT32 index2 = 0;
		for( index2=0; index2<rlDevices.validDeviceCount; index2++ )
		{
			rlRequest.pbFrameType = RS_RL_FRAME_ALL;
			RelayStreamReceiver *rlStreamReceiver = new RelayStreamReceiver( recorder );
			rlStreamReceiver->SetStreamInfo( rlDevices.deviceInfo[index2].GetID() );
			rlRequest.pReceiver = rlStreamReceiver;
			rlStreamReceiver->Start();
			sValue = recorder->StartRelay( &rlDevices.deviceInfo[index2], &rlRequest );

			if( sValue )
			{
				CScopedLock lock( &_lockOfRlReceiver );
				std::map<CString,RLStreamReceiverList>::iterator rlStreamIter;
				rlStreamIter = _rlUUID.find( chInfoClean->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
				if( rlStreamIter==_rlUUID.end() )
				{
					RLStreamReceiverList rlStreamReceiverList;
					rlStreamReceiverList.insert( std::make_pair(groupUUID,static_cast<IStreamReceiver5*>(rlRequest.pReceiver)) );
					_rlUUID.insert( std::make_pair(chInfoClean->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, rlStreamReceiverList) );
				}
				else
				{

					RLStreamReceiverList rlStreamReceiverList = rlStreamIter->second;
					std::pair<std::multimap<CString,IStreamReceiver5*>::iterator, std::multimap<CString,IStreamReceiver5*>::iterator> rlStreamIterPair;
					rlStreamIterPair = rlStreamReceiverList.equal_range( groupUUID );
					if( rlStreamIterPair.first==rlStreamIterPair.second )
					{
						rlStreamReceiverList.insert( std::make_pair(groupUUID,static_cast<IStreamReceiver5*>(rlRequest.pReceiver)) );
						rlStreamIter->second = rlStreamReceiverList;
					}
					else
					{
						BOOL bFound = FALSE;
						std::multimap<CString,IStreamReceiver5*>::iterator rlSteamReceiverIter;
						for( rlSteamReceiverIter=rlStreamIterPair.first; rlSteamReceiverIter!=rlStreamIterPair.second; rlSteamReceiverIter++ )
						{
							RelayStreamReceiver *tmpRLStreamReceiver = static_cast<RelayStreamReceiver*>( rlSteamReceiverIter->second );
							if( tmpRLStreamReceiver->GetStreamInfo()==rlStreamReceiver->GetStreamInfo() )
							{
								tmpRLStreamReceiver->Stop();
								RS_RELAY_INFO_T rlInfo;
								rlInfo.pReceiver = tmpRLStreamReceiver;
								BOOL sValue = tmpRLStreamReceiver->GetRecorder()->StopRelay( &rlInfo );
								do { ::Sleep( 10 ); } while( tmpRLStreamReceiver->IsConnected() );
								delete tmpRLStreamReceiver;

								bFound = TRUE;
								break;
							}
						}

						if( bFound ) rlStreamReceiverList.erase( rlSteamReceiverIter );
						rlStreamReceiverList.insert( std::make_pair(groupUUID,static_cast<IStreamReceiver5*>(rlRequest.pReceiver)) );
						rlStreamIter->second = rlStreamReceiverList;
					}
					rlStreamIter->second = rlStreamReceiverList;
				}
			}
			else
			{
				delete ( static_cast<RelayStreamReceiver*>(rlStreamReceiver) );
			}
		}
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::StartRelay( CChannelInfo *chInfo )
{
	Recorder * recorder = GetRecorder( chInfo );
	if( !recorder ) return FALSE;

	BOOL value = TRUE;
	chInfo->m_pCamera->StartRelay();

	RS_DEVICE_INFO_T	rlDevice;
	RS_RELAY_REQUEST_T	rlRequest;
	if( !MakeDeviceInfo(chInfo, &rlDevice) ) return FALSE;

	rlRequest.pbFrameType = RS_RL_FRAME_ALL;
	RelayStreamReceiver *rlStreamReceiver = new RelayStreamReceiver( recorder );
	rlStreamReceiver->SetStreamInfo( rlDevice.GetID() );
	rlRequest.pReceiver = rlStreamReceiver;
	rlStreamReceiver->Start();
	value = recorder->StartRelay( &rlDevice, &rlRequest );
	if( value )
	{
		CScopedLock lock( &_lockOfRlReceiver );
		std::map<CString,RLStreamReceiverList>::iterator rlStreamIter;
		rlStreamIter = _rlUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
		if( rlStreamIter==_rlUUID.end() )
		{
			RLStreamReceiverList rlStreamReceiverList;
			rlStreamReceiverList.insert( std::make_pair(chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid,static_cast<IStreamReceiver5*>(rlRequest.pReceiver)) );
			_rlUUID.insert( std::make_pair(chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, rlStreamReceiverList) );
		}
		else
		{

			RLStreamReceiverList rlStreamReceiverList = rlStreamIter->second;
			std::pair<std::multimap<CString,IStreamReceiver5*>::iterator, std::multimap<CString,IStreamReceiver5*>::iterator> rlStreamIterPair;
			rlStreamIterPair = rlStreamReceiverList.equal_range( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid );
			if( rlStreamIterPair.first==rlStreamIterPair.second )
			{
				rlStreamReceiverList.insert( std::make_pair(chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid,static_cast<IStreamReceiver5*>(rlRequest.pReceiver)) );
				rlStreamIter->second = rlStreamReceiverList;
			}
			else
			{
				BOOL bFound = FALSE;
				std::multimap<CString,IStreamReceiver5*>::iterator rlSteamReceiverIter;
				for( rlSteamReceiverIter=rlStreamIterPair.first; rlSteamReceiverIter!=rlStreamIterPair.second; rlSteamReceiverIter++ )
				{
					RelayStreamReceiver *tmpRLStreamReceiver = static_cast<RelayStreamReceiver*>( rlSteamReceiverIter->second );
					if( tmpRLStreamReceiver->GetStreamInfo()==rlStreamReceiver->GetStreamInfo() )
					{
						tmpRLStreamReceiver->Stop();
						RS_RELAY_INFO_T rlInfo;
						rlInfo.pReceiver = tmpRLStreamReceiver;
						BOOL sValue = tmpRLStreamReceiver->GetRecorder()->StopRelay( &rlInfo );
						do { ::Sleep( 10 ); } while( tmpRLStreamReceiver->IsConnected() );
						delete tmpRLStreamReceiver;

						bFound = TRUE;
						break;
					}
				}

				if( bFound ) rlStreamReceiverList.erase( rlSteamReceiverIter );
				rlStreamReceiverList.insert( std::make_pair(chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid,static_cast<IStreamReceiver5*>(rlRequest.pReceiver)) );
				rlStreamIter->second = rlStreamReceiverList;
			}
			rlStreamIter->second = rlStreamReceiverList;
		}
	}
	else
	{
		delete ( static_cast<RelayStreamReceiver*>(rlStreamReceiver) );
	}

	return value;
}
	
BOOL RecorderIF::UpdateRelay( CGroupInfo *group )
{


	return FALSE;
}

BOOL RecorderIF::UpdateRelay( CString groupUUID, std::vector<CChannelInfo*> *chInfos )
{


	return FALSE;
}

BOOL RecorderIF::UpdateRelay( CChannelInfo *chInfo )
{


	return FALSE;
}

BOOL RecorderIF::StopRelay( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();
	for( index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}	
	return StopRelay( group->GetGroupUUID(), &chInfos );
}

BOOL RecorderIF::StopRelay( CString groupUUID, std::vector<CChannelInfo*> *chInfos )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;
	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		CChannelInfo *chInfo;
		BOOL sValue = FALSE;

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		if( chGrpRelatedRcrdIterPair.first==chGrpRelatedRcrdIterPair.second ) continue;
		chGrpRelatedRcrdIter = chGrpRelatedRcrdIterPair.first;
		chInfo = chGrpRelatedRcrdIter->second;

		{
			CScopedLock lock( &_lockOfRlReceiver );

			std::map<CString, RLStreamReceiverList>::iterator iter = _rlUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
			if( iter!=_rlUUID.end() )
			{
				RLStreamReceiverList rlStreamReceiverList = iter->second;
				std::pair<RLStreamReceiverList::iterator, RLStreamReceiverList::iterator> rlStreamIterPair = rlStreamReceiverList.equal_range( groupUUID );
				RLStreamReceiverList::iterator rlStreamIter;
				for( rlStreamIter=rlStreamIterPair.first; rlStreamIter!=rlStreamIterPair.second; rlStreamIter++ )
				{
					RelayStreamReceiver *rlStreamReceiver = static_cast<RelayStreamReceiver*>( rlStreamIter->second );
					rlStreamReceiver->Stop();
					RS_RELAY_INFO_T rlInfo;
					rlInfo.pReceiver = rlStreamReceiver;
					sValue = rlStreamReceiver->GetRecorder()->StopRelay( &rlInfo );
					do { ::Sleep( 10 ); } while( rlStreamReceiver->IsConnected() );
					delete rlStreamReceiver;
					rlStreamIter->second = NULL;
				}
				rlStreamReceiverList.erase( groupUUID );
				_rlUUID.erase( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
				_rlUUID.insert( std::make_pair(chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, rlStreamReceiverList) );
			}
		}
		value &= sValue;
	}
	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		(*iter)->m_pCamera->StopRelay();
	}
	return value;
}

BOOL RecorderIF::StopRelay( CChannelInfo *chInfo )
{
	BOOL value = TRUE;
	RS_DEVICE_INFO_T	rlDevice;
	//RS_RELAY_REQUEST_T	rlRequest;

	if( !MakeDeviceInfo(chInfo, &rlDevice) ) return FALSE;

	{
		CScopedLock lock( &_lockOfRlReceiver );

		std::map<CString, RLStreamReceiverList>::iterator iter = _rlUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
		if( iter!=_rlUUID.end() )
		{
			RLStreamReceiverList rlStreamReceiverList = iter->second;
			std::pair<RLStreamReceiverList::iterator, RLStreamReceiverList::iterator> rlStreamIterPair = rlStreamReceiverList.equal_range( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid );
			RLStreamReceiverList::iterator rlStreamIter;
			for( rlStreamIter=rlStreamIterPair.first; rlStreamIter!=rlStreamIterPair.second; rlStreamIter++ )
			{
				RelayStreamReceiver *rlStreamReceiver = static_cast<RelayStreamReceiver*>( rlStreamIter->second );
				rlStreamReceiver->Stop();
				RS_RELAY_INFO_T rlInfo;
				rlInfo.pReceiver = rlStreamReceiver;
				value = rlStreamReceiver->GetRecorder()->StopRelay( &rlInfo );
				do { ::Sleep( 10 ); } while( rlStreamReceiver->IsConnected() );
				delete rlStreamReceiver;
				rlStreamIter->second = NULL;
			}
			rlStreamReceiverList.erase( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid );
			_rlUUID.erase( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
			_rlUUID.insert( std::make_pair(chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, rlStreamReceiverList) );
		}
	}
	chInfo->m_pCamera->StopRelay();
	return value;
}

///////////////// RECORDING
BOOL RecorderIF::GetRecordingScheduleList( CGroupInfo *group, RS_RECORD_SCHEDULE_SET_T *recordShcedList )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}
	return GetRecordingScheduleList( &chInfos, recordShcedList );
}

BOOL RecorderIF::GetRecordingScheduleList( std::vector<CChannelInfo*> *chInfos, RS_RECORD_SCHEDULE_SET_T *recordShcedList )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = FALSE;
		CChannelInfo *chInfoClean;
		CChannelInfo *chInfo;

		RS_DEVICE_INFO_SET_T *deviceInfoList = new RS_DEVICE_INFO_SET_T;
		RS_RECORD_SCHEDULE_SET_T *rcrdShcedList = new RS_RECORD_SCHEDULE_SET_T;

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			 chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;

			chInfoClean = chInfo;
			if( !MakeDeviceInfo(chInfo, &deviceInfoList->deviceInfo[index]) ) continue;
			size++;
		}
		deviceInfoList->validDeviceCount = size;
		
		Recorder * recorder = GetRecorder( chInfoClean );
		if( !recorder ) 
		{
			delete deviceInfoList;
			delete rcrdShcedList;
			continue;
		}

		sValue = recorder->GetRecordingScheduleList( deviceInfoList, recordShcedList );
		if( sValue )
		{
			UINT validSchedCount = recordShcedList->validScheduleCount;
			for( UINT index2=0; index2<rcrdShcedList->validScheduleCount; index2++ )
			{
				recordShcedList->scheduleInfos[validSchedCount+index2] = rcrdShcedList->scheduleInfos[index2];
			}
			recordShcedList->validScheduleCount +=rcrdShcedList->validScheduleCount;
		}
		delete deviceInfoList;
		delete rcrdShcedList;
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::GetRecordingScheduleList( CChannelInfo *chInfo, RS_RECORD_SCHEDULE_SET_T *recordShcedList )
{
	Recorder * recorder = GetRecorder( chInfo );
	if( !recorder ) return FALSE;

	RS_DEVICE_INFO_SET_T deviceInfoList;
	deviceInfoList.validDeviceCount = 1;
	if( !MakeDeviceInfo(chInfo, &deviceInfoList.deviceInfo[0]) ) return FALSE;
	return recorder->GetRecordingScheduleList( &deviceInfoList, recordShcedList );
}

BOOL RecorderIF::UpdateRecordingSchedule( CGroupInfo *group, UINT schedType, BOOL audio, 
										  UINT bitSun, UINT bitMon, UINT bitTue, UINT bitWed, UINT bitThu, UINT bitFri, UINT bitSat, 	
										  UINT preRecordingTime, UINT postRecordingTime )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}
	return UpdateRecordingSchedule( &chInfos, schedType, audio, bitSun, bitMon, bitTue, bitWed, bitThu, bitFri, bitSat, preRecordingTime, postRecordingTime );
}

BOOL RecorderIF::UpdateRecordingSchedule( std::vector<CChannelInfo*> *chInfos, UINT schedType, BOOL audio, 
										  UINT bitSun, UINT bitMon, UINT bitTue, UINT bitWed, UINT bitThu, UINT bitFri, UINT bitSat, 	
										  UINT preRecordingTime, UINT postRecordingTime )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = FALSE;
		CChannelInfo *chInfoClean;
		CChannelInfo *chInfo;



		RS_RECORD_SCHEDULE_SET_T *recSchedList = new RS_RECORD_SCHEDULE_SET_T();
		RS_RESPONSE_INFO_SET_T *rspInfoList = new RS_RESPONSE_INFO_SET_T();
		//memset( recSchedList, 0x00, sizeof(RS_RECORD_SCHEDULE_SET_T) );
		//memset( rspInfoList, 0x00, sizeof(RS_RESPONSE_INFO_SET_T) );
		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			 chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;

			chInfoClean = chInfo;

			if( !MakeSchedInfo(chInfo, &recSchedList->scheduleInfos[index], schedType, audio, bitSun, bitMon, bitTue, bitWed, bitThu, bitFri, bitSat, preRecordingTime, postRecordingTime) ) continue;
			size++;
		}
		recSchedList->validScheduleCount = size;

		Recorder *recorder = GetRecorder( chInfoClean );
		if( !recorder )
		{
			delete recSchedList;
			delete rspInfoList;
			continue;
		}

		sValue = recorder->UpdateRecordingSchedule( recSchedList, rspInfoList );
		delete recSchedList;
		delete rspInfoList;
		value &= sValue;
	}
	return value;
}


/*
	UINT nAudio;				// Audio recording 유무
	UINT nSun_bitflag;		// sunday의 시간별 recording 유무를 나타내는 24bit bitflag (1bit->1시간)
	UINT nMon_bitflag;		// monday의 시간별 recording 유무를 나타내는 24bit bitflag (1bit->1시간)
	UINT nTue_bitflag;		// thesday의 시간별 recording 유무를 나타내는 24bit bitflag (1bit->1시간)
	UINT nWed_bitflag;		// wednesday의 시간별 recording 유무를 나타내는 24bit bitflag (1bit->1시간)
	UINT nThu_bitflag;		// thursday의 시간별 recording 유무를 나타내는 24bit bitflag (1bit->1시간)
	UINT nFri_bitflag;		// friday의 시간별 recording 유무를 나타내는 24bit bitflag (1bit->1시간)
	UINT nSat_bitflag;		// saturday의 시간별 recording 유무를 나타내는 24bit bitflag (1bit->1시간)

*/

BOOL RecorderIF::UpdateRecordingSchedule( CChannelInfo *chInfo, UINT schedType, BOOL audio, 
										  UINT bitSun, UINT bitMon, UINT bitTue, UINT bitWed, UINT bitThu, UINT bitFri, UINT bitSat, 
										  UINT preRecordingTime, UINT postRecordingTime )
{
	Recorder *recorder = GetRecorder( chInfo );
	if( !recorder ) return FALSE;

	RS_RECORD_SCHEDULE_SET_T recSchedList;
	RS_RESPONSE_INFO_SET_T rspInfoList = {0,};
	recSchedList.validScheduleCount = 1;
	if( !MakeSchedInfo(chInfo, &recSchedList.scheduleInfos[0], schedType, audio,  bitSun, bitMon, bitTue, bitWed, bitThu, bitFri, bitSat, preRecordingTime, postRecordingTime) ) return FALSE;
	return recorder->UpdateRecordingSchedule( &recSchedList, &rspInfoList );
}

BOOL RecorderIF::SetRecordingOverwrite( CGroupInfo *group, BOOL onoff )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}
	return SetRecordingOverwrite( &chInfos, onoff );
}

BOOL RecorderIF::SetRecordingOverwrite( std::vector<CChannelInfo*> *chInfos, BOOL onoff )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = FALSE;
		CChannelInfo *chInfo;

		RS_RECORD_OVERWRITE_INFO_T overwriteInfo = {0,};
		RS_RESPONSE_INFO_T responseInfo = {0,};

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			 chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;
			break;
		}
		overwriteInfo.onoff = onoff==TRUE?1:0;
		Recorder *recorder = GetRecorder( chInfo );
		if( !recorder ) continue;
		sValue = recorder->SetRecordingOverwrite( &overwriteInfo, &responseInfo );
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::SetRecordingOverwrite( RS_SERVER_INFO_T *serverInfo, BOOL onoff )
{
	Recorder *recorder = GetRecorder( serverInfo );
	if( !recorder ) return FALSE;

	RS_RECORD_OVERWRITE_INFO_T overwriteInfo = {0,};
	RS_RESPONSE_INFO_T responseInfo = {0,};
	overwriteInfo.onoff = onoff==TRUE?1:0;
	return recorder->SetRecordingOverwrite( &overwriteInfo, &responseInfo );
}

BOOL RecorderIF::GetRecordingOverwrite( RS_SERVER_INFO_T *serverInfo, BOOL *onoff )
{
	Recorder *recorder = GetRecorder( serverInfo );
	if( !recorder ) return FALSE;

	RS_RECORD_OVERWRITE_INFO_T overwriteInfo = {0,};
	RS_RESPONSE_INFO_T responseInfo = {0,};
	if( recorder->GetRecordingOverwrite(&overwriteInfo) )
	{
		if( overwriteInfo.onoff ) (*onoff) = TRUE;
		else (*onoff) = FALSE;
		return TRUE;
	}
	else return FALSE;
}

BOOL RecorderIF::SetRecordingRetentionTime( CGroupInfo *group, BOOL enable, UINT year, UINT month, UINT week, UINT day )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}
	return SetRecordingRetentionTime( &chInfos, enable, year, month, week, day );
}

BOOL RecorderIF::SetRecordingRetentionTime( std::vector<CChannelInfo*> *chInfos, BOOL enable, UINT year, UINT month, UINT week, UINT day )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = FALSE;
		CChannelInfo *chInfo;

		RS_RECORD_RETENTION_INFO_T retentionInfo;
		RS_RESPONSE_INFO_T responseInfo = {0,};

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			 chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;
			break;
		}
		retentionInfo.enable = enable==TRUE?1:0;
		retentionInfo.year = year;
		retentionInfo.month = month;
		retentionInfo.week = week;
		retentionInfo.day = day;

		Recorder *recorder = GetRecorder( chInfo );
		if( !recorder ) continue;

		sValue = recorder->SetRecordingRetentionTime( &retentionInfo, &responseInfo );
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::SetRecordingRetentionTime( RS_SERVER_INFO_T *serverInfo, BOOL enable, UINT year, UINT month, UINT week, UINT day )
{
	Recorder *recorder = GetRecorder( serverInfo );
	if( !recorder ) return FALSE;

	RS_RECORD_RETENTION_INFO_T retentionInfo;
	RS_RESPONSE_INFO_T responseInfo;
	retentionInfo.enable = enable==TRUE?1:0;
	retentionInfo.year = year;
	retentionInfo.month = month;
	retentionInfo.week = week;
	retentionInfo.day = day;

	return recorder->SetRecordingRetentionTime( &retentionInfo, &responseInfo );
}

BOOL RecorderIF::GetRecordingRetentionTime( RS_SERVER_INFO_T *serverInfo, BOOL *enable, UINT *year, UINT *month, UINT *week, UINT *day )
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

BOOL RecorderIF::GetDiskInfo( RS_SERVER_INFO_T *serverInfo, RS_DISK_INFO_SET_T *diskInfoList )
{
	Recorder *recorder = GetRecorder( serverInfo );
	if( !recorder ) return FALSE;
	return recorder->GetDiskInfo( diskInfoList );
}

BOOL RecorderIF::ReserveDiskSpace( RS_SERVER_INFO_T *serverInfo, CString strVolumeSerial, UINT64 recordingSize )
{
	Recorder *recorder = GetRecorder( serverInfo );
	if( !recorder ) return FALSE;

	RS_DISK_INFO_SET_T diskInfoList;
	RS_DEVICE_INFO_SET_T deviceInfoList;
	RS_DISK_RESPONSE_SET_T diskRspInfoList;
	deviceInfoList.validDeviceCount = 1;
	RS_DISK_INFO_SET_T diskInfoList4CheckingVolumeSerial;
	recorder->GetDiskInfo( &diskInfoList4CheckingVolumeSerial );
	UINT64 commiteReserved = 0;
	UINT index = 0;
	for( index=0; index<diskInfoList4CheckingVolumeSerial.validDiskCount; index++ )
	{
		if( diskInfoList4CheckingVolumeSerial.diskInfo[index].strVolumeSerial==strVolumeSerial ) break;
	}

	if( index>=diskInfoList4CheckingVolumeSerial.validDiskCount ) return FALSE;
	diskInfoList.validDiskCount = 1;
	diskInfoList.diskInfo[0].strVolumeSerial = strVolumeSerial;
	diskInfoList.diskInfo[0].nCommitReserved = recordingSize;
	
	return recorder->ReserveDiskSpace( &diskInfoList, &diskRspInfoList );
}

BOOL RecorderIF::GetDiskPolicy( RS_SERVER_INFO_T *serverInfo, std::map<CString,VCamUUIDList> *vcamUUIDList )
{
	Recorder *recorder = GetRecorder( serverInfo );
	if( !recorder ) return FALSE;

	RS_DEVICE_INFO_SET_T devInfoList;
	if( recorder->GetDeviceList(&devInfoList) )
	{
		if( devInfoList.validDeviceCount<1 ) return FALSE;
	}
	else return FALSE;

	RS_DISK_POLICY_SET_T diskPolicyList;
	if( recorder->GetDiskPolicy(&diskPolicyList) )
	{
		if( diskPolicyList.validDiskCount<1 ) return FALSE;
		for( UINT index=0; index<diskPolicyList.validDiskCount; index++ )
		{
			//현재 등록된 카메라의 MAC주소와 이와 연관된 실제 UUID값을 상기 추출된 장치정보(devInfoList)와 비교하여 업데이트 한다.
			for( UINT index2=0; index2<diskPolicyList.diskPolicyInfo[index].validDeviceCount; index2++ )
			{
				for( UINT index3=0; index3<devInfoList.validDeviceCount; index3++ )
				{
					if( devInfoList.deviceInfo[index3].GetMAC()==diskPolicyList.diskPolicyInfo[index].strMAC[index2] )
					{
						std::map<CString,VCamUUIDList>::iterator iter;
						iter = vcamUUIDList->find( diskPolicyList.diskPolicyInfo[index].strVolumeSerial );
						//해당 strVolumeSerial과 연관된 카메라 UUID가 존재하지 않을 경우
						if( iter==vcamUUIDList->end() )
						{
							VCamUUIDList vcamUUIDs;
							vcamUUIDs.push_back( devInfoList.deviceInfo[index3].GetID() );
							vcamUUIDList->insert( std::make_pair(diskPolicyList.diskPolicyInfo[index].strVolumeSerial,vcamUUIDs) );
						}
						else
						{
							VCamUUIDList vcamUUIDs = iter->second;
							vcamUUIDs.push_back( devInfoList.deviceInfo[index3].GetID() );
							vcamUUIDList->erase( iter );
							vcamUUIDList->insert( std::make_pair(diskPolicyList.diskPolicyInfo[index].strVolumeSerial,vcamUUIDs) );
						}
					}
				}
			}
		}
		return TRUE;
	}
	else 
		return FALSE;
}

BOOL RecorderIF::UpdateDiskPolicy( CChannelManager *chMgr, CString strVolumeSerial )
{
	std::vector<CChannelInfo*> chInfos;
	for( INT i=0; i<gChannelManager->GetCount(); i++ )
	{
		CChannelInfo *chInfo = chMgr->GetByIndex( i );
		chInfos.push_back( chInfo );
	}
	return UpdateDiskPolicy( &chInfos, strVolumeSerial );
}

BOOL RecorderIF::UpdateDiskPolicy( CGroupInfo *group, CString strVolumeSerial )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}
	return UpdateDiskPolicy( &chInfos, strVolumeSerial );
}

BOOL RecorderIF::UpdateDiskPolicy( std::vector<CChannelInfo*> *chInfos, CString strVolumeSerial )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = FALSE;
		CChannelInfo *chInfoClean;
		CChannelInfo *chInfo;

		RS_DISK_INFO_SET_T *diskInfoList = new RS_DISK_INFO_SET_T;
		RS_DEVICE_INFO_SET_T *deviceInfoList = new RS_DEVICE_INFO_SET_T;
		RS_DISK_RESPONSE_SET_T *diskRspInfoList = new RS_DISK_RESPONSE_SET_T;

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			 chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;
			chInfoClean = chInfo;

			if( !MakeDeviceInfo(chInfo, &deviceInfoList->deviceInfo[index]) ) continue;
			size++;
		}
		deviceInfoList->validDeviceCount = size;

		Recorder *recorder = GetRecorder( chInfoClean );
		if( !recorder )
		{
			delete diskInfoList;
			delete deviceInfoList;
			delete diskRspInfoList;
			continue;
		}

		RS_DISK_INFO_SET_T *diskInfoList2 = new RS_DISK_INFO_SET_T;
		recorder->GetDiskInfo( diskInfoList2 );

		UINT64 commiteReserved = 0;
		for( index=0; index<diskInfoList2->validDiskCount; index++ )
		{
			if( diskInfoList2->diskInfo[index].strVolumeSerial==strVolumeSerial )
			{
				commiteReserved = diskInfoList2->diskInfo[index].nCommitReserved;
				break;
			}
		}

		if( index==diskInfoList2->validDiskCount ) value &= sValue;

		diskInfoList->validDiskCount = 1;
		diskInfoList->diskInfo[0].strVolumeSerial = strVolumeSerial;
		diskInfoList->diskInfo[0].nCommitReserved = commiteReserved;


		RS_DEVICE_INFO_SET_T *deviceInfoList2 = new RS_DEVICE_INFO_SET_T;
		recorder->GetDeviceList( deviceInfoList2 );

		for( UINT16 index2=0; index2<deviceInfoList2->validDeviceCount; index2++ )
		{
			CString regDevUUID = deviceInfoList2->deviceInfo[index2].GetID();
			CString regDevURL = deviceInfoList2->deviceInfo[index2].GetURL();
			CString regDevMac = deviceInfoList2->deviceInfo[index2].GetMAC();
			TRACE( _T("regDevUUID = %s, regDevURL = %s, regDevMac = %s \n"), regDevUUID, regDevURL, regDevMac );
			for( UINT16 index3=0; index3<deviceInfoList->validDeviceCount; index3++ )
			{
				if( (deviceInfoList->deviceInfo[index3].GetID()==regDevUUID) && (deviceInfoList->deviceInfo[index3].GetURL()==regDevURL) )
				{
					deviceInfoList->deviceInfo[index3].SetMAC( regDevMac );
				}
			}
		}
		delete deviceInfoList2;

		sValue = recorder->UpdateDiskPolicy( diskInfoList, deviceInfoList, diskRspInfoList );
		delete diskInfoList;
		delete deviceInfoList;
		delete diskRspInfoList;
		delete diskInfoList2;
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::UpdateDiskPolicy( CChannelInfo *chInfo, CString strVolumeSerial  )
{
	Recorder *recorder = GetRecorder( chInfo );
	if( !recorder ) return FALSE;

	RS_DISK_INFO_SET_T diskInfoList;
	RS_DEVICE_INFO_SET_T deviceInfoList;
	RS_DISK_RESPONSE_SET_T diskRspInfoList;
	deviceInfoList.validDeviceCount = 1;
	if( !MakeDeviceInfo(chInfo, &deviceInfoList.deviceInfo[0]) ) return FALSE;

	RS_DISK_INFO_SET_T diskInfoList4CheckingVolumeSerial;
	recorder->GetDiskInfo( &diskInfoList4CheckingVolumeSerial );

	UINT64 commiteReserved = 0;
	UINT index = 0;
	for( index=0; index<diskInfoList4CheckingVolumeSerial.validDiskCount; index++ )
	{
		if( diskInfoList4CheckingVolumeSerial.diskInfo[index].strVolumeSerial==strVolumeSerial )
		{
			commiteReserved = diskInfoList4CheckingVolumeSerial.diskInfo[index].nCommitReserved;
			break;
		}
	}

	if( index==diskInfoList4CheckingVolumeSerial.validDiskCount ) return FALSE;
	diskInfoList.validDiskCount = 1;
	diskInfoList.diskInfo[0].strVolumeSerial = strVolumeSerial;
	diskInfoList.diskInfo[0].nCommitReserved = commiteReserved;
	return recorder->UpdateDiskPolicy( &diskInfoList, &deviceInfoList, &diskRspInfoList );
}

BOOL RecorderIF::IsRecording( CGroupInfo *group, RS_RECORDING_STATUS_SET_T *recordingStatusList )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo)
			{
				if(chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdAssigned)
					chInfos.push_back( chInfo );
			}
		}
	}
	return IsRecording( &chInfos, recordingStatusList );
}

BOOL RecorderIF::IsRecording( std::vector<CChannelInfo*> *chInfos, RS_RECORDING_STATUS_SET_T *recordingStatusList )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		BOOL sValue = FALSE;
		CChannelInfo *chInfo;
		CChannelInfo *chInfoClean;
		UINT32 size = 0;
		UINT16 index = 0;

		RS_DEVICE_INFO_SET_T *devInfoList = new RS_DEVICE_INFO_SET_T;
		RS_RECORDING_STATUS_SET_T *rcrdStatusList = new RS_RECORDING_STATUS_SET_T;

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;

			chInfoClean = chInfo;
			if( !MakeDeviceInfo(chInfo, &devInfoList->deviceInfo[index]) ) continue;
			size++;
		}
		devInfoList->validDeviceCount = size;

		Recorder *recorder = GetRecorder( chInfo );
		if( !recorder )
		{
			delete devInfoList;
			delete rcrdStatusList;
			continue;
		}
		sValue = recorder->IsRecording( devInfoList, rcrdStatusList );
		if( sValue )
		{
			UINT validCount = recordingStatusList->validDeviceCount;
			for( UINT index2=0; index2<rcrdStatusList->validDeviceCount; index2++ )
			{
				recordingStatusList->recordingStatus[validCount+index2] = rcrdStatusList->recordingStatus[index2];
			}
			recordingStatusList->validDeviceCount += rcrdStatusList->validDeviceCount;
		}
		value &= sValue;
		delete devInfoList;
		delete rcrdStatusList;
	}
	return value;
}

BOOL RecorderIF::IsRecording( CChannelInfo *chInfo, RS_RECORDING_STATUS_SET_T *recordingStatusList )
{
	Recorder *recorder = GetRecorder( chInfo );
	if( !recorder ) return FALSE;

	RS_DEVICE_INFO_SET_T *devInfoList = new RS_DEVICE_INFO_SET_T;
	if( !MakeDeviceInfo(chInfo, &devInfoList->deviceInfo[0]) ) return FALSE;
	devInfoList->validDeviceCount = 1;
	return recorder->IsRecording( devInfoList, recordingStatusList );
}

BOOL RecorderIF::StartRecordingRequest( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}
	return StartRecordingRequest( &chInfos );
}

BOOL RecorderIF::StartRecordingRequest( std::vector<CChannelInfo*> *chInfos )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		BOOL sValue = FALSE;
		CChannelInfo *chInfo;
		
		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first; 
			 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			 chGrpRelatedRcrdIter++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;

			RS_DEVICE_INFO_SET_T devInfoList;
			devInfoList.validDeviceCount = 1;
			RS_RESPONSE_INFO_SET_T rspInfoList;
			RS_DEVICE_STATUS_SET_T devStatusList;

			if( !MakeDeviceInfo(chInfo, &devInfoList.deviceInfo[0]) ) continue;
			Recorder *recorder = GetRecorder( chInfo );
			if( !recorder ) continue;

			sValue = recorder->StartRecordingRequest( &devInfoList, &rspInfoList );
			value &= sValue;
		}
	}
	return value;
}

BOOL RecorderIF::StartRecordingRequest( CChannelInfo *chInfo )
{
	Recorder *recorder = GetRecorder( chInfo );
	if( !recorder ) return FALSE;

	RS_DEVICE_INFO_SET_T devInfoList;
	devInfoList.validDeviceCount = 1;
	RS_RESPONSE_INFO_SET_T rspInfoList;
	RS_DEVICE_STATUS_SET_T devStatusList;

	if( !MakeDeviceInfo(chInfo, &devInfoList.deviceInfo[0]) ) return FALSE;
	return recorder->StartRecordingRequest( &devInfoList, &rspInfoList );
}

BOOL RecorderIF::StopRecordingRequest( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}
	return StopRecordingRequest( &chInfos );
}

BOOL RecorderIF::StopRecordingRequest( std::vector<CChannelInfo*> *chInfos )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		BOOL sValue = FALSE;
		CChannelInfo *chInfo;

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first; 
			 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			 chGrpRelatedRcrdIter++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;

			RS_DEVICE_INFO_SET_T devInfoList;
			devInfoList.validDeviceCount = 1;
			RS_RESPONSE_INFO_SET_T rspInfoList;
			RS_DEVICE_STATUS_SET_T devStatusList;

			if( !MakeDeviceInfo(chInfo, &devInfoList.deviceInfo[0]) ) continue;
			Recorder *recorder = GetRecorder( chInfo );
			if( !recorder ) continue;

			sValue = recorder->StopRecordingRequest( &devInfoList, &rspInfoList );
			value &= sValue;
		}
	}
	return value;
}

BOOL RecorderIF::StopRecordingRequest( CChannelInfo *chInfo )
{
	Recorder *recorder = GetRecorder( chInfo );
	if( !recorder ) return FALSE;

	RS_DEVICE_INFO_SET_T devInfoList;
	devInfoList.validDeviceCount = 1;
	RS_RESPONSE_INFO_SET_T rspInfoList;
	RS_DEVICE_STATUS_SET_T devStatusList;

	if( !MakeDeviceInfo(chInfo, &devInfoList.deviceInfo[0]) ) return FALSE;
	return recorder->StopRecordingRequest( &devInfoList, &rspInfoList );
}

BOOL RecorderIF::StartRecordingAll( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}
	return StartRecordingAll( &chInfos );
}

BOOL RecorderIF::StartRecordingAll( std::vector<CChannelInfo*> *chInfos )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin();
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end();
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = FALSE;
		CChannelInfo *chInfo;

		RS_DEVICE_INFO_SET_T devInfoList;
		RS_RESPONSE_INFO_SET_T rspInfoList;

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			 chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;
			break;
		}

		Recorder *recorder = GetRecorder( chInfo );
		if( !recorder ) continue;
		sValue = recorder->StartRecordingAll();
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::StartRecordingAll( RS_SERVER_INFO_T *serverInfo )
{
	Recorder *recorder = GetRecorder( serverInfo );
	if( !recorder ) return FALSE;
	return recorder->StartRecordingAll();
}

BOOL RecorderIF::StopRecordingAll( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}

	}
	return StopRecordingAll( &chInfos );
}

BOOL RecorderIF::StopRecordingAll( std::vector<CChannelInfo*> *chInfos )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = FALSE;
		CChannelInfo *chInfo;

		RS_DEVICE_INFO_SET_T devInfoList;
		RS_RESPONSE_INFO_SET_T rspInfoList;

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			 chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;
			break;
		}

		Recorder *recorder = GetRecorder( chInfo );
		if( !recorder ) continue;
		sValue = recorder->StopRecordingAll();
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::StopRecordingAll( RS_SERVER_INFO_T *serverInfo )
{
	Recorder *recorder = GetRecorder( serverInfo );
	if( !recorder ) return FALSE;
	return recorder->StopRecordingAll();
}

BOOL RecorderIF::DeleteRecordingData( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}
	return DeleteRecordingData( &chInfos );
}

BOOL RecorderIF::DeleteRecordingData( std::vector<CChannelInfo*> *chInfos )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = FALSE;
		CChannelInfo *chInfoClean;
		CChannelInfo *chInfo;

		RS_DEVICE_INFO_SET_T devInfoList;
		RS_DELETE_RESPONSE_SET_T delRspList;

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			 chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;
			chInfoClean = chInfo;

			if( !MakeDeviceInfo(chInfo, &devInfoList.deviceInfo[index]) ) continue;
			size++;
		}
		devInfoList.validDeviceCount = size;

		Recorder *recorder = GetRecorder( chInfoClean );
		if( !recorder ) continue;
		sValue = recorder->DeleteRecordingData( &devInfoList, &delRspList );
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::DeleteRecordingData( CChannelInfo *chInfo )
{
	Recorder *recorder = GetRecorder( chInfo );
	if( !recorder ) return FALSE;

	RS_DEVICE_INFO_SET_T devInfoList;
	RS_DELETE_RESPONSE_SET_T delRspList;
	devInfoList.validDeviceCount = 1;
	if( !MakeDeviceInfo(chInfo, &devInfoList.deviceInfo[0]) ) return FALSE;
	return recorder->DeleteRecordingData( &devInfoList, &delRspList );
}

BOOL RecorderIF::GetTimeIndex( CGroupInfo *group, UINT year, std::map<CChannelInfo*,CPtrArray> *time )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}
	return GetTimeIndex( &chInfos, year, time );
}

BOOL RecorderIF::GetTimeIndex( std::vector<CChannelInfo*> *chInfos, UINT year, std::map<CChannelInfo*,CPtrArray> *time )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}


	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = FALSE;
		CChannelInfo *chInfoClean;
		CChannelInfo *chInfo;

		RS_PLAYBACK_DEVICE_SET_T	pbDeviceList;
		RS_SEARCH_REQUEST_T			searchRequest;
		RS_SEARCH_RESPONSE_SET_T	*searchRspList = new RS_SEARCH_RESPONSE_SET_T();

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			 chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;
			chInfoClean = chInfo;
			if( !MakeDeviceInfo(chInfo, &pbDeviceList.deviceInfo[index]) ) continue;
			size++;
		}
		pbDeviceList.validDeviceCount = size;
		Recorder *recorder = GetRecorder( chInfoClean );
		if( !recorder ) 
		{
			delete searchRspList;
			continue;
		}
		searchRequest.year = year;
		recorder->GetYearIndex( &pbDeviceList, &searchRequest, searchRspList );
		for( UINT monthIndex=0; monthIndex<RS_MAX_MONTH; monthIndex++ )
		{
			searchRequest.month = monthIndex+1;
			recorder->GetMonthIndex( &pbDeviceList, &searchRequest, searchRspList );
			for( UINT dayIndex=0; dayIndex<RS_MAX_DAY; dayIndex++ )
			{
				searchRequest.day = dayIndex+1;
				recorder->GetDayIndex( &pbDeviceList, &searchRequest, searchRspList );
				for( UINT hourIndex=0; hourIndex<RS_MAX_HOUR; hourIndex++ )
				{
					searchRequest.hour = hourIndex+1;
					recorder->GetHourIndex( &pbDeviceList, &searchRequest, searchRspList );
				}
			}
		}
		delete searchRspList;
	}
	return value;
}

BOOL RecorderIF::GetDayIndex( CGroupInfo *group, UINT year, UINT month, UINT day )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}
	return GetDayIndex( &chInfos, year, month, day );
}

BOOL RecorderIF::GetDayIndex( std::vector<CChannelInfo*> *chInfos, UINT year, UINT month, UINT day )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}


	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = FALSE;
		CChannelInfo *chInfoClean;
		CChannelInfo *chInfo;

		RS_PLAYBACK_DEVICE_SET_T	pbDeviceList;
		RS_SEARCH_REQUEST_T			searchRequest;
		RS_SEARCH_RESPONSE_SET_T	*searchRspList = new RS_SEARCH_RESPONSE_SET_T();

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			 chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;
			chInfoClean = chInfo;
			if( !MakeDeviceInfo(chInfo, &pbDeviceList.deviceInfo[index]) ) continue;
			size++;
		}
		pbDeviceList.validDeviceCount = size;
		Recorder * recorder = GetRecorder( chInfoClean );
		if( !recorder )
		{
			delete searchRspList;
			continue;
		}
		searchRequest.year = year;
		searchRequest.month = month;
		searchRequest.day = day;
		searchRequest.hour = 17;

		recorder->GetDayIndex( &pbDeviceList, &searchRequest, searchRspList );
		for( UINT hourIndex=0; hourIndex<RS_MAX_HOUR; hourIndex++ )
		{
			searchRequest.hour = hourIndex+1;
			recorder->GetHourIndex( &pbDeviceList, &searchRequest, searchRspList );
		}

		delete searchRspList;
	}
	return value;
}

BOOL RecorderIF::GetDayIndex( CChannelInfo *chInfo, UINT year, UINT month, UINT day )
{
	Recorder *recorder = GetRecorder( chInfo );
	if( !recorder ) return FALSE;

	RS_PLAYBACK_DEVICE_SET_T	pbDeviceList;
	if( !MakeDeviceInfo(chInfo, &pbDeviceList.deviceInfo[0]) ) return FALSE;

	RS_SEARCH_REQUEST_T			searchRequest;
	pbDeviceList.validDeviceCount = 1;
	searchRequest.year = year;
	searchRequest.month = month;
	searchRequest.day = day;

	RS_SEARCH_RESPONSE_SET_T	*searchRspList = new RS_SEARCH_RESPONSE_SET_T();

	if( !recorder->GetDayIndex(&pbDeviceList, &searchRequest, searchRspList) )
	{
		delete searchRspList;
		return FALSE;
	}
	for( UINT hourIndex=0; hourIndex<RS_MAX_HOUR; hourIndex++ )
	{
		searchRequest.hour = hourIndex;
		recorder->GetHourIndex( &pbDeviceList, &searchRequest, searchRspList );
	}

	RECORD_TIME_INFO recordTime;
	for( UINT hourIndex=0; hourIndex<RS_MAX_HOUR; hourIndex++ )
	{
		if( searchRspList->responseInfo[0].months[month-1].days[day-1].bHours[hourIndex]==TRUE )
		{
			for( UINT minIndex=0; minIndex<RS_MAX_MINUTE; minIndex++ )
			{
				if( searchRspList->responseInfo[0].months[month-1].days[day-1].hours[hourIndex].bMinutes[minIndex]==TRUE )
				{
					if( recordTime.startTime.year==0 ) //시작 시간 설정
					{
						recordTime.startTime.year = year;
						recordTime.startTime.month = month;
						recordTime.startTime.day = day;
						recordTime.startTime.hour = hourIndex;
						recordTime.startTime.minute = minIndex;
					}
					/*
					else //종료 시간 적합성 검사 및 설정
					{
						//lastDayOfMonth
						RECORD_TIME_INFO *pRecordTime = new RECORD_TIME_INFO( recordTime );
						if( !chInfo->m_pRecTime ) chInfo->m_pRecTime = new CPtrArray;
						chInfo->m_pRecTime->Add( pRecordTime );
						recordTime.startTime.year = 0;
						recordTime.startTime.month = 0;
						recordTime.startTime.hour = 0;
						recordTime.startTime.minute = 0;
						recordTime.startTime.second = 0;
						recordTime.endTime.year = 0;
						recordTime.endTime.month = 0;
						recordTime.endTime.hour = 0;
						recordTime.endTime.minute = 0;
						recordTime.endTime.second = 0;
					}
					*/
				}
				else //현재 레코딩 시간이 없으나 이전 설정에 시작 시간이 있다면 바로 전시간(minute)의 값을 종료시간으로 설정
				{
					if( recordTime.startTime.year!=0 )
					{
						if( minIndex==0 ) //전 시간의 마지막 분이 종료 시각임
						{
							recordTime.endTime.year = year;
							recordTime.endTime.month = month;

							if( day==1 ) // 달의 첫번째 요일이면, 이전달의 마지막 날이 시작일이 됌.
								recordTime.endTime.day = lastDayOfMonth[month];
							else
								recordTime.endTime.day = day;

							recordTime.endTime.hour = hourIndex;
							recordTime.endTime.minute = RS_MAX_MINUTE-1;

							// 2012-02-01 00::00::00
							// 2012-01-31 23::59::00
						}
						else //현재 분의 -1을 제거한 값이 종료 시각임
						{
							recordTime.endTime.year = year;
							recordTime.endTime.month = month;
							recordTime.endTime.day = day;
							recordTime.endTime.hour = hourIndex;
							recordTime.endTime.minute = minIndex;
						}

						//시작 시간 및 종료시간을 모두 구했으므로, 이를 CChannelInfo의 pRecTime 포인트 배열에 신규 생성 후 할당한다.
						RECORD_TIME_INFO *pRecordTime = new RECORD_TIME_INFO( recordTime );

						if( !chInfo->m_pRecTime ) 
							chInfo->m_pRecTime = new CPtrArray;

						chInfo->m_pRecTime->Add( pRecordTime );
						
						recordTime.startTime.year = 0;
						recordTime.startTime.month = 0;
						recordTime.startTime.hour = 0;
						recordTime.startTime.minute = 0;
						recordTime.startTime.second = 0;
						recordTime.endTime.year = 0;
						recordTime.endTime.month = 0;
						recordTime.endTime.hour = 0;
						recordTime.endTime.minute = 0;
						recordTime.endTime.second = 0;
					}
				}
			}
		}
		else
		{
			if( recordTime.startTime.year!=0 )
			{
				recordTime.endTime.year = year;
				recordTime.endTime.month = month;
				recordTime.endTime.day = day;
				recordTime.endTime.hour = hourIndex;
				recordTime.endTime.minute = 0;

				//시작 시간 및 종료시간을 모두 구했으므로, 이를 CChannelInfo의 pRecTime 포인트 배열에 신규 생성 후 할당한다.
				RECORD_TIME_INFO *pRecordTime = new RECORD_TIME_INFO( recordTime );

				if( !chInfo->m_pRecTime ) 
					chInfo->m_pRecTime = new CPtrArray;

				chInfo->m_pRecTime->Add( pRecordTime );

				recordTime.startTime.year = 0;
				recordTime.startTime.month = 0;
				recordTime.startTime.hour = 0;
				recordTime.startTime.minute = 0;
				recordTime.startTime.second = 0;
				recordTime.endTime.year = 0;
				recordTime.endTime.month = 0;
				recordTime.endTime.hour = 0;
				recordTime.endTime.minute = 0;
				recordTime.endTime.second = 0;
			}
		}
	}

	delete searchRspList;
	return TRUE;
}

BOOL RecorderIF::GetPeriodIndex( CChannelInfo *chInfo, int period)
{
	Recorder *recorder = GetRecorder( chInfo );
	if( !recorder ) return FALSE;

	RS_PLAYBACK_DEVICE_SET_T	pbDeviceList;
	if( !MakeDeviceInfo(chInfo, &pbDeviceList.deviceInfo[0]) ) return FALSE;

	RS_SEARCH_RESPONSE_SET_T	*searchRspList = new RS_SEARCH_RESPONSE_SET_T();
	long aaa=sizeof(RS_SEARCH_RESPONSE_SET_T);

	RS_SEARCH_REQUEST_T searchRequest;
	pbDeviceList.validDeviceCount = 1;
	
	CTime curTime = CTime::GetCurrentTime();
	int year, month, day;
	CTime preTime;

	RECORD_TIME_INFO recordTime;
	//memset(&recordTime, 0x00, sizeof(RECORD_TIME_INFO));

	for(int p=0; p<period; p++)
	{
		preTime = curTime - CTimeSpan( (period-p-1), 0, 0, 0 );

		year = searchRequest.year = preTime.GetYear();
		month = searchRequest.month = preTime.GetMonth();
		day = searchRequest.day = preTime.GetDay();

		if( !recorder->GetDayIndex(&pbDeviceList, &searchRequest, searchRspList) )
		{
			delete searchRspList;
			return FALSE;
		}

		for( UINT hourIndex=0; hourIndex<RS_MAX_HOUR; hourIndex++ )
		{
			searchRequest.hour = hourIndex;
			recorder->GetHourIndex( &pbDeviceList, &searchRequest, searchRspList );
		}

		for( UINT hourIndex=0; hourIndex<RS_MAX_HOUR; hourIndex++ )
		{
			if( searchRspList->responseInfo[0].months[month-1].days[day-1].bHours[hourIndex]==TRUE )
			{
				for( UINT minIndex=0; minIndex<RS_MAX_MINUTE; minIndex++ )
				{
					if( searchRspList->responseInfo[0].months[month-1].days[day-1].hours[hourIndex].bMinutes[minIndex]==TRUE )
					{
						if( recordTime.startTime.year==0 ) //시작 시간 설정
						{
							recordTime.startTime.year = year;
							recordTime.startTime.month = month;
							recordTime.startTime.day = day;
							recordTime.startTime.hour = hourIndex;
							recordTime.startTime.minute = minIndex;
						}
					}
					else //현재 레코딩 시간이 없으나 이전 설정에 시작 시간이 있다면 바로 전시간(minute)의 값을 종료시간으로 설정
					{
						if( recordTime.startTime.year!=0 )
						{
							if( minIndex==0 ) //전 시간의 마지막 분이 종료 시각임
							{
								recordTime.endTime.year = year;
								recordTime.endTime.month = month;

								if( day==1 ) // 달의 첫번째 요일이면, 이전달의 마지막 날이 시작일이 됌.
									recordTime.endTime.day = lastDayOfMonth[month];
								else
									recordTime.endTime.day = day;

								recordTime.endTime.hour = hourIndex;
								recordTime.endTime.minute = RS_MAX_MINUTE-1;

								// 2012-02-01 00::00::00
								// 2012-01-31 23::59::00
							}
							else //현재 분의 -1을 제거한 값이 종료 시각임
							{
								recordTime.endTime.year = year;
								recordTime.endTime.month = month;
								recordTime.endTime.day = day;
								recordTime.endTime.hour = hourIndex;
								recordTime.endTime.minute = minIndex;
							}

							//시작 시간 및 종료시간을 모두 구했으므로, 이를 CChannelInfo의 pRecTime 포인트 배열에 신규 생성 후 할당한다.
							RECORD_TIME_INFO *pRecordTime = new RECORD_TIME_INFO( recordTime );

							if( !chInfo->m_pRecTime ) 
								chInfo->m_pRecTime = new CPtrArray;

							chInfo->m_pRecTime->Add( pRecordTime );
							recordTime.startTime.year = 0;
							recordTime.startTime.month = 0;
							recordTime.startTime.hour = 0;
							recordTime.startTime.minute = 0;
							recordTime.startTime.second = 0;
							recordTime.endTime.year = 0;
							recordTime.endTime.month = 0;
							recordTime.endTime.hour = 0;
							recordTime.endTime.minute = 0;
							recordTime.endTime.second = 0;
						}
					}
				}
			}
			else
			{
				if( recordTime.startTime.year!=0 )
				{
					recordTime.endTime.year = year;
					recordTime.endTime.month = month;
					recordTime.endTime.day = day;
					recordTime.endTime.hour = hourIndex;
					recordTime.endTime.minute = 0;

					//시작 시간 및 종료시간을 모두 구했으므로, 이를 CChannelInfo의 pRecTime 포인트 배열에 신규 생성 후 할당한다.
					RECORD_TIME_INFO *pRecordTime = new RECORD_TIME_INFO( recordTime );

					if( !chInfo->m_pRecTime ) 
						chInfo->m_pRecTime = new CPtrArray;

					chInfo->m_pRecTime->Add( pRecordTime );
					recordTime.startTime.year = 0;
					recordTime.startTime.month = 0;
					recordTime.startTime.hour = 0;
					recordTime.startTime.minute = 0;
					recordTime.startTime.second = 0;
					recordTime.endTime.year = 0;
					recordTime.endTime.month = 0;
					recordTime.endTime.hour = 0;
					recordTime.endTime.minute = 0;
					recordTime.endTime.second = 0;
				}
			}
		}
	}
	delete searchRspList;
	return TRUE;
}

BOOL RecorderIF::GetRecentMinIndex( CChannelInfo *chInfo, CTime curTime, UINT periodMin ) //min=1,2,3,4,5,6,10,12,15,20,30,60
{
	Recorder *recorder = GetRecorder( chInfo );
	if( !recorder ) return FALSE;

	RS_PLAYBACK_DEVICE_SET_T	pbDeviceList;
	if( !MakeDeviceInfo(chInfo, &pbDeviceList.deviceInfo[0]) ) return FALSE;

	RS_SEARCH_RESPONSE_SET_T	*searchRspList = new RS_SEARCH_RESPONSE_SET_T();
	RS_SEARCH_REQUEST_T searchRequest;
	pbDeviceList.validDeviceCount = 1;

	CTime startTime, endTime;
	int endMin=curTime.GetMinute()-(curTime.GetMinute()%periodMin);
	endTime=CTime(curTime.GetYear(), curTime.GetMonth(),curTime.GetDay(),curTime.GetHour(), endMin, 0);
	startTime = endTime - CTimeSpan( 0, 0, periodMin, 0 );

	searchRequest.year = startTime.GetYear();
	searchRequest.month = startTime.GetMonth();
	searchRequest.day = startTime.GetDay();

	if( !recorder->GetDayIndex(&pbDeviceList, &searchRequest, searchRspList) )
	{
		delete searchRspList;
		return FALSE;
	}

	searchRequest.hour = startTime.GetHour();
	if( !recorder->GetHourIndex(&pbDeviceList, &searchRequest, searchRspList) )
	{
		delete searchRspList;
		return FALSE;
	}

	UINT hourIndex=startTime.GetHour();
	RECORD_TIME_INFO recordTime;
	if( searchRspList->responseInfo[0].months[startTime.GetMonth()-1].days[startTime.GetDay()-1].bHours[hourIndex]==TRUE )
	{
		int startMin, endMin;
		startMin=startTime.GetMinute();
		if(startTime.GetHour()==curTime.GetHour())
			endMin=curTime.GetMinute();
		else
			endMin=RS_MAX_MINUTE-1;;

		for( INT minIndex=startMin; minIndex<endMin; minIndex++ )
		{
			if( searchRspList->responseInfo[0].months[startTime.GetMonth()-1].days[startTime.GetDay()-1].hours[hourIndex].bMinutes[minIndex]==TRUE )
			{
				if( recordTime.startTime.year==0 ) //시작 시간 설정
				{
					recordTime.startTime.year = startTime.GetYear();
					recordTime.startTime.month = startTime.GetMonth();
					recordTime.startTime.day = startTime.GetDay();
					recordTime.startTime.hour = hourIndex;
					recordTime.startTime.minute = minIndex;
				}
			}
			else
			{
				if( recordTime.startTime.year!=0 )
				{
					recordTime.endTime.year = startTime.GetYear();
					recordTime.endTime.month = startTime.GetMonth();
					recordTime.endTime.day = startTime.GetDay();
					recordTime.endTime.hour = hourIndex;
					recordTime.endTime.minute = 0;

					//시작 시간 및 종료시간을 모두 구했으므로, 이를 CChannelInfo의 pRecTime 포인트 배열에 신규 생성 후 할당한다.
					RECORD_TIME_INFO *pRecordTime = new RECORD_TIME_INFO( recordTime );

					if( !chInfo->m_pRecTime ) 
						chInfo->m_pRecTime = new CPtrArray;

					chInfo->m_pRecTime->Add( pRecordTime );
				}
			}
		}
		if( recordTime.startTime.year!=0 )
		{
			recordTime.endTime.year = endTime.GetYear();
			recordTime.endTime.month = endTime.GetMonth();
			recordTime.endTime.day = endTime.GetDay();
			recordTime.endTime.hour = endTime.GetHour();
			recordTime.endTime.minute = endTime.GetMinute();

			//시작 시간 및 종료시간을 모두 구했으므로, 이를 CChannelInfo의 pRecTime 포인트 배열에 신규 생성 후 할당한다.
			RECORD_TIME_INFO *pRecordTime = new RECORD_TIME_INFO( recordTime );

			if( !chInfo->m_pRecTime ) 
				chInfo->m_pRecTime = new CPtrArray;

			chInfo->m_pRecTime->Add( pRecordTime );
		}
	}

	delete searchRspList;
	return TRUE;
}

int RecorderIF::GetRecordedTimeExport( CChannelInfo *chInfo, CTime startTime, CTime endTime ) //min=1,2,3,4,5,6,10,12,15,20,30,60
{
	Recorder *recorder = GetRecorder( chInfo );
	if( !recorder ) return RECORDER_CONNECTION_FAIL;

	RS_PLAYBACK_DEVICE_SET_T	pbDeviceList;
	if( !MakeDeviceInfo(chInfo, &pbDeviceList.deviceInfo[0]) ) return RECORDER_CONNECTION_FAIL;

	RS_SEARCH_RESPONSE_SET_T	*searchRspList = new RS_SEARCH_RESPONSE_SET_T();
	RS_SEARCH_REQUEST_T searchRequest;
	pbDeviceList.validDeviceCount = 1;

	//CTime curTime = CTime::GetCurrentTime();
	int year, month, day;
	RECORD_TIME_INFO recordTime;
	memset(&recordTime, 0x00, sizeof(RECORD_TIME_INFO));

	year = searchRequest.year = startTime.GetYear();
	month = searchRequest.month = startTime.GetMonth();
	day = searchRequest.day = startTime.GetDay();

	if( !recorder->GetDayIndex(&pbDeviceList, &searchRequest, searchRspList) )
	{
		delete searchRspList;
		return RECORDER_NOT_EXIST_EXPORT_FILE;
	}
	for( UINT hourIndex=0; hourIndex<RS_MAX_HOUR; hourIndex++ )
	{
		searchRequest.hour = hourIndex;
		recorder->GetHourIndex( &pbDeviceList, &searchRequest, searchRspList );
	}

	for( UINT hourIndex=0; hourIndex<RS_MAX_HOUR; hourIndex++ )
	{
		if( searchRspList->responseInfo[0].months[month-1].days[day-1].bHours[hourIndex]==TRUE )
		{
			for( UINT minIndex=0; minIndex<RS_MAX_MINUTE; minIndex++ )
			{
				if( searchRspList->responseInfo[0].months[month-1].days[day-1].hours[hourIndex].bMinutes[minIndex]==TRUE )
				{
					delete searchRspList;
					return RECORDER_EXPORT_OK;
				}
			}
		}
	}

	delete searchRspList;
	return RECORDER_NOT_EXIST_EXPORT_FILE;
}

/*
BOOL RecorderIF::GetYearIndex( CGroupInfo *group, UINT year, std::map<CChannelInfo*,PLAYBACK_TIME_INFO> *month )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
		chInfos.push_back( chInfo );
	}
	return GetYearIndex( &chInfos, year, month );
}

BOOL RecorderIF::GetYearIndex( std::vector<CChannelInfo*> *chInfos, UINT year, std::map<CChannelInfo*,PLAYBACK_TIME_INFO> *month )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = FALSE;
		CChannelInfo *chInfoClean;
		CChannelInfo *chInfo;
		RS_PLAYBACK_DEVICE_SET_T pbDeviceList;
		RS_SEARCH_REQUEST_T searchRequest;
		searchRequest.year = year;
		RS_SEARCH_RESPONSE_SET_T searchRspList;

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			 chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;

			chInfoClean = chInfo;

			if( !MakeDeviceInfo(chInfo, &pbDeviceList.deviceInfo[index]) ) continue;
			size++;
		}
		pbDeviceList.validDeviceCount = size;
		searchRspList.validDeviceCount = size;

		Recorder * recorder = GetRecorder( chInfoClean );
		sValue = recorder->GetYearIndex( &pbDeviceList, &searchRequest, &searchRspList );
		if( sValue )
		{
			for( index=0; index<pbDeviceList.validDeviceCount; index++ )
			{
				chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
				for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first; 
					 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
					 chGrpRelatedRcrdIter++ )
				{
					chInfo = chGrpRelatedRcrdIter->second;
					if( chInfo==NULL ) continue;

					if( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid==pbDeviceList.deviceInfo[index].GetID() ) break;
				}
				PlayBackTimeInfo pbTimeInfo;
				for( UINT index2=0; index2<RS_MAX_MONTH; index2++ )
				{
					if( searchRspList.responseInfo[index].months[index2]==1 ) 
						pbTimeInfo.bMonths[index2] = TRUE;
				}
				month->insert( std::make_pair(chInfo, pbTimeInfo) );
			}
		}
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::GetMonthIndex( CGroupInfo *group, UINT year, UINT month, std::map<CChannelInfo*,PlayBackMonthInfo> *day )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
		chInfos.push_back( chInfo );
	}
	return GetMonthIndex( &chInfos, year, month, day );
}

BOOL RecorderIF::GetMonthIndex( std::vector<CChannelInfo*> *chInfos, UINT year, UINT month, std::map<CChannelInfo*,PlayBackMonthInfo> *day )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = FALSE;
		CChannelInfo *chInfoClean;
		CChannelInfo *chInfo;
		RS_PLAYBACK_DEVICE_SET_T pbDeviceList;
		RS_SEARCH_REQUEST_T searchRequest;
		searchRequest.year = year;
		searchRequest.month = month;
		RS_SEARCH_RESPONSE_SET_T searchRspList;

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			 chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;

			chInfoClean = chInfo;

			if( !MakeDeviceInfo(chInfo, &pbDeviceList.deviceInfo[index]) ) continue;
			size++;
		}
		pbDeviceList.validDeviceCount = size;
		searchRspList.validDeviceCount = size;

		Recorder * recorder = GetRecorder( chInfoClean );
		sValue = recorder->GetMonthIndex( &pbDeviceList, &searchRequest, &searchRspList );
		if( sValue )
		{
			for( index=0; index<pbDeviceList.validDeviceCount; index++ )
			{
				chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
				for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first; 
					 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
					 chGrpRelatedRcrdIter++ )
				{
					chInfo = chGrpRelatedRcrdIter->second;
					if( chInfo==NULL ) continue;

					if( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid==pbDeviceList.deviceInfo[index].GetID() ) break;
				}
				PlayBackMonthInfo pbTimeInfo;
				for( UINT index2=0; index2<RS_MAX_DAY; index2++ )
				{
					if( searchRspList.responseInfo[index].days[index2]==1 ) 
						pbTimeInfo.bDays[index2] = TRUE;
				}
				day->insert( std::make_pair(chInfo, pbTimeInfo) );
			}
		}
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::GetDayIndex( CGroupInfo *group,  UINT year, UINT month, UINT day, std::map<CChannelInfo*,PlayBackDayInfo> *hour )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
		chInfos.push_back( chInfo );
	}
	return GetDayIndex( &chInfos, year, month, day, hour );
}

BOOL RecorderIF::GetDayIndex( std::vector<CChannelInfo*> *chInfos, UINT year, UINT month, UINT day, std::map<CChannelInfo*,PlayBackDayInfo> *hour )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = FALSE;
		CChannelInfo *chInfoClean;
		CChannelInfo *chInfo;
		RS_PLAYBACK_DEVICE_SET_T pbDeviceList;
		RS_SEARCH_REQUEST_T searchRequest;
		searchRequest.year = year;
		searchRequest.month = month;
		searchRequest.day = day;
		RS_SEARCH_RESPONSE_SET_T searchRspList;

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			 chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;

			chInfoClean = chInfo;

			if( !MakeDeviceInfo(chInfo, &pbDeviceList.deviceInfo[index]) ) continue;
			size++;
		}
		pbDeviceList.validDeviceCount = size;
		searchRspList.validDeviceCount = size;

		Recorder * recorder = GetRecorder( chInfoClean );
		sValue = recorder->GetDayIndex( &pbDeviceList, &searchRequest, &searchRspList );
		if( sValue )
		{
			for( index=0; index<pbDeviceList.validDeviceCount; index++ )
			{
				chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
				for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first; 
					 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
					 chGrpRelatedRcrdIter++ )
				{
					chInfo = chGrpRelatedRcrdIter->second;
					if( chInfo==NULL ) continue;

					if( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid==pbDeviceList.deviceInfo[index].GetID() ) break;
				}
				PlayBackDayInfo pbTimeInfo;
				for( UINT index2=0; index2<RS_MAX_HOUR; index2++ )
				{
					if( searchRspList.responseInfo[index].hours[index2]==1 ) 
						pbTimeInfo.bHours[index2] = TRUE;
				}

				hour->insert( std::make_pair(chInfo, pbTimeInfo) );
			}
		}
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::GetHourIndex( CGroupInfo *group, UINT year, UINT month, UINT day, UINT hour, std::map<CChannelInfo*,PlayBackHourInfo> *minute )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
		chInfos.push_back( chInfo );
	}
	return GetHourIndex( &chInfos, year, month, day, hour, minute );
}

BOOL RecorderIF::GetHourIndex( std::vector<CChannelInfo*> *chInfos, UINT year, UINT month, UINT day, UINT hour, std::map<CChannelInfo*,PlayBackHourInfo> *minute )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = FALSE;
		CChannelInfo *chInfoClean;
		CChannelInfo *chInfo;
		RS_PLAYBACK_DEVICE_SET_T pbDeviceList;
		RS_SEARCH_REQUEST_T searchRequest;
		searchRequest.year = year;
		searchRequest.month = month;
		searchRequest.day = day;
		searchRequest.hour = hour;
		RS_SEARCH_RESPONSE_SET_T searchRspList;

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			 chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;

			chInfoClean = chInfo;

			if( !MakeDeviceInfo(chInfo, &pbDeviceList.deviceInfo[index]) ) continue;
			size++;
		}
		pbDeviceList.validDeviceCount = size;
		searchRspList.validDeviceCount = size;

		Recorder * recorder = GetRecorder( chInfoClean );
		sValue = recorder->GetHourIndex( &pbDeviceList, &searchRequest, &searchRspList );
		if( sValue )
		{
			for( index=0; index<pbDeviceList.validDeviceCount; index++ )
			{
				chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
				for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first; 
					 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
					 chGrpRelatedRcrdIter++ )
				{
					chInfo = chGrpRelatedRcrdIter->second;
					if( chInfo==NULL ) continue;

					if( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid==pbDeviceList.deviceInfo[index].GetID() ) break;
				}
				PlayBackHourInfo pbTimeInfo;
				for( UINT index2=0; index2<RS_MAX_MINUTE; index2++ )
				{
					if( searchRspList.responseInfo[index].minutes[index2]==1 ) 
						pbTimeInfo.bMinutes[index2] = TRUE;
				}

				minute->insert( std::make_pair(chInfo, pbTimeInfo) );
			}
		}
		value &= sValue;
	}
	return value;
}
*/

BOOL RecorderIF::StartPlayback( CGroupInfo *group, UINT year, UINT month, UINT day, UINT hour, UINT minute, UINT second )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();

	CCameraDlg *pCameraDlg;
	CCamera *pCamera;
	for( UINT index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo)
			{
				chInfos.push_back( chInfo );
				pCamera = chInfo->m_pCamera;
				//pCameraDlg = g_2DDlgManager->Get2DDlgInfo(index)->m_pCameraDlg;
				//pCamera->m_pCameraDlg = pCameraDlg;
				//pCameraDlg->m_pCamera = pCamera;
				////pCamera->m_flag_playback_start = TRUE;
#ifdef VMS_RECORDER
				if(pCamera->m_flag_relay_on)
				{
					g_Recorder->StopRelay( chInfo );
					pCamera->StopPlayBack();
				}
#endif
				pCamera->StartPlayBack();
			}
		}

	}
	return StartPlayback( group->GetGroupUUID(), &chInfos, year, month, day, hour, minute, second );
}

BOOL RecorderIF::StartPlayback( CString groupUUID, std::vector<CChannelInfo*> *chInfos, UINT year, UINT month, UINT day, UINT hour, UINT minute, UINT second )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		BOOL sValue = FALSE;
		CChannelInfo *chInfoClean;
		CChannelInfo *chInfo;
		RS_PLAYBACK_DEVICE_SET_T pbDevices = {0,};
		RS_PLAYBACK_REQUEST_T pbRequest;
		RS_PLAYBACK_INFO_T pbInfo = {0,};
		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		for( chGrpRelatedRcrdIter=chGrpRelatedRcrdIterPair.first, index=0; 
			 chGrpRelatedRcrdIter!=chGrpRelatedRcrdIterPair.second; 
			 chGrpRelatedRcrdIter++, index++ )
		{
			chInfo = chGrpRelatedRcrdIter->second;
			if( chInfo==NULL ) continue;

			chInfoClean = chInfo;

			if( !MakeDeviceInfo(chInfo, &pbDevices.deviceInfo[index]) ) continue;
			
			size++;
		}
		pbDevices.validDeviceCount = size;

		pbRequest.year = year;
		pbRequest.month = month;
		pbRequest.day = day;
		pbRequest.hour = hour;
		pbRequest.minute = minute;
		pbRequest.second = second;
		pbRequest.pbDirection = RS_PLAYBACK_DIRECTION_T_FORWARD;
		pbRequest.pbFrameType = RS_PB_FRAME_ALL;

		Recorder *recorder = GetRecorder( chInfoClean );
		if( !recorder ) continue;

		PlayBackStreamReceiver *pbStreamReceiver = new PlayBackStreamReceiver( recorder );
		UINT32 index2 = 0;
		for( index2=0; index2<pbDevices.validDeviceCount; index2++ )
		{
			pbStreamReceiver->AddStreamInfo( index2, pbDevices.deviceInfo[index2].GetID() );
		}
		pbStreamReceiver->Start();
		pbRequest.pReceiver = pbStreamReceiver;
		sValue = recorder->StartPlayback( &pbDevices, &pbRequest, &pbInfo );
		if( sValue )
		{
			CScopedLock lock( &_lockOfPbReceiver );
			std::map<CString,PBStreamReceiverList>::iterator iter;
			iter = _pbUUID.find( chInfoClean->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
			if( iter==_pbUUID.end() )
			{
				PBStreamReceiverList pbStreamReceiverList;
				pbStreamReceiverList.insert( std::make_pair(groupUUID,static_cast<IStreamReceiver5*>(pbRequest.pReceiver)) );
				_pbUUID.insert( std::make_pair(chInfoClean->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, pbStreamReceiverList) );
			}
			else
			{
				PBStreamReceiverList pbStreamReceiverList = iter->second;
				PBStreamReceiverList::iterator iter2;
				iter2 = pbStreamReceiverList.find( groupUUID );
				if( iter2==pbStreamReceiverList.end() )
				{
					pbStreamReceiverList.insert( std::make_pair(groupUUID,static_cast<IStreamReceiver5*>(pbRequest.pReceiver)) );
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
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::StartPlayback( CChannelInfo *chInfo, UINT year, UINT month, UINT day, UINT hour, UINT minute, UINT second )
{	
	Recorder *recorder = GetRecorder( chInfo );
	if( !recorder ) return FALSE;

	BOOL value = FALSE;
	RS_PLAYBACK_DEVICE_SET_T pbDevices = {0,};
	RS_PLAYBACK_REQUEST_T pbRequest;
	RS_PLAYBACK_INFO_T pbInfo = {0,};

	if( !MakeDeviceInfo(chInfo, &pbDevices.deviceInfo[0]) ) return FALSE;
	pbDevices.validDeviceCount = 1;
	pbRequest.year = year;
	pbRequest.month = month;
	pbRequest.day = day;
	pbRequest.hour = hour;
	pbRequest.minute = minute;
	pbRequest.second = second;
	pbRequest.pbDirection = RS_PLAYBACK_DIRECTION_T_FORWARD;
	pbRequest.pbFrameType = RS_PB_FRAME_ALL;

	PlayBackStreamReceiver *pbStreamReceiver = new PlayBackStreamReceiver( recorder );
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
		chInfo->m_pCamera->StartPlayBack();/////
		CScopedLock lock( &_lockOfPbReceiver );
		std::map<CString,PBStreamReceiverList>::iterator iter;
		iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
		if( iter==_pbUUID.end() )
		{
			PBStreamReceiverList pbStreamReceiverList;
			pbStreamReceiverList.insert( std::make_pair(chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid, static_cast<IStreamReceiver5*>(pbRequest.pReceiver)) );
			_pbUUID.insert( std::make_pair(chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, pbStreamReceiverList) );
		}
		else
		{
			PBStreamReceiverList pbStreamReceiverList = iter->second;
			PBStreamReceiverList::iterator iter2;
			iter2 = pbStreamReceiverList.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid );
			if( iter2==pbStreamReceiverList.end() )
			{
				pbStreamReceiverList.insert( std::make_pair(chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid, static_cast<IStreamReceiver5*>(pbRequest.pReceiver)) );
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

BOOL RecorderIF::StopPlayback( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();
	for( index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}

	}	
	return StopPlayback( group->GetGroupUUID(), &chInfos );
}

BOOL RecorderIF::StopPlayback( CString groupUUID, std::vector<CChannelInfo*> *chInfos )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;
	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		UINT32 size = 0;
		UINT16 index = 0;
		CChannelInfo *chInfo;
		BOOL sValue = FALSE;

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		if( chGrpRelatedRcrdIterPair.first==chGrpRelatedRcrdIterPair.second ) continue;
		chGrpRelatedRcrdIter = chGrpRelatedRcrdIterPair.first;
		chInfo = chGrpRelatedRcrdIter->second;

		{
			CScopedLock lock( &_lockOfPbReceiver );

			std::map<CString, PBStreamReceiverList>::iterator iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
			if( iter!=_pbUUID.end() )
			{
				PBStreamReceiverList pbStreamReceiverList = iter->second;
				PBStreamReceiverList::iterator iter2 = pbStreamReceiverList.find( groupUUID );
				if( iter2!=pbStreamReceiverList.end() )
				{
					PlayBackStreamReceiver *pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
					pbStreamReceiver->Stop();
					RS_PLAYBACK_INFO_T pbInfo = {0,};
					pbInfo.pReceiver = pbStreamReceiver;
					pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
					sValue = pbStreamReceiver->GetRecorder()->StopPlayback( &pbInfo );
					do { ::Sleep( 10 ); } while( pbStreamReceiver->IsConnected() );
					delete pbStreamReceiver;
					iter2->second = NULL;
					pbStreamReceiverList.erase( groupUUID );
				}
				_pbUUID.erase( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
				_pbUUID.insert( std::make_pair(chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, pbStreamReceiverList) );
			}
		}
		value &= sValue;
	}
	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		(*iter)->m_pCamera->StopPlayBack();
	}
	return value;
}

BOOL RecorderIF::StopPlayback( CChannelInfo *chInfo )
{
	BOOL value = FALSE;
	{
		CScopedLock lock( &_lockOfPbReceiver );

		std::map<CString, PBStreamReceiverList>::iterator iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
		if( iter!=_pbUUID.end() )
		{
			PBStreamReceiverList pbStreamReceiverList = iter->second;
			PBStreamReceiverList::iterator iter2 = pbStreamReceiverList.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid );
			if( iter2!=pbStreamReceiverList.end() )
			{
				PlayBackStreamReceiver *pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
				pbStreamReceiver->Stop();
				RS_PLAYBACK_INFO_T pbInfo = {0,};
				pbInfo.pReceiver = pbStreamReceiver;
				pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
				value = pbStreamReceiver->GetRecorder()->StopPlayback( &pbInfo );
				do { ::Sleep( 10 ); } while( pbStreamReceiver->IsConnected() );
				delete pbStreamReceiver;
				iter2->second = NULL;
				pbStreamReceiverList.erase( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid );
			}
			_pbUUID.erase( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
			_pbUUID.insert( std::make_pair(chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, pbStreamReceiverList) );
		}
	}
	chInfo->m_pCamera->StopPlayBack();
	return value;
}

BOOL RecorderIF::ControlPlay( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();
	for( index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}	
	return ControlPlay( group->GetGroupUUID(), &chInfos );
}

BOOL RecorderIF::ControlPlay( CString groupUUID, std::vector<CChannelInfo*> *chInfos )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		BOOL sValue = FALSE;
		CChannelInfo *chInfo;

		RS_PLAYBACK_INFO_T pbInfo = {0,};

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		chGrpRelatedRcrdIter = chGrpRelatedRcrdIterPair.first;
		if( chGrpRelatedRcrdIter==chGrpRelatedRcrdIterPair.second ) continue;
		chInfo = chGrpRelatedRcrdIter->second;
		if( chInfo )
		{
			CScopedLock lock( &_lockOfPbReceiver );
			std::map<CString,PBStreamReceiverList>::iterator iter;
			iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
			if( iter==_pbUUID.end() ) sValue = FALSE;
			else
			{
				PBStreamReceiverList pbStreamReceiverList = iter->second;
				PBStreamReceiverList::iterator iter2;
				iter2 = pbStreamReceiverList.find( groupUUID );
				if( iter2==pbStreamReceiverList.end() ) sValue = FALSE;
				else
				{
					PlayBackStreamReceiver *pbStreamReceiver;
					pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
					if( pbStreamReceiver )
					{
						Recorder *recorder = GetRecorder( chInfo );
						if( !recorder ) continue;

						pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
						pbInfo.pbDirection = RS_PLAYBACK_DIRECTION_T_FORWARD;
						pbInfo.pbSpeed = RS_PLAYBACK_SPEED_T_1X;
						sValue = recorder->ControlPlay( &pbInfo );
					}
					else sValue = FALSE;
				}
			}
		}
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::ControlPlay( CChannelInfo *chInfo )
{
	BOOL value = FALSE;
	RS_PLAYBACK_INFO_T pbInfo = {0,};
	if( chInfo )
	{
		CScopedLock lock( &_lockOfPbReceiver );
		std::map<CString,PBStreamReceiverList>::iterator iter;
		iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
		if( iter==_pbUUID.end() ) value = FALSE;
		else
		{
			PBStreamReceiverList pbStreamReceiverList = iter->second;
			PBStreamReceiverList::iterator iter2;
			iter2 = pbStreamReceiverList.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid );
			if( iter2==pbStreamReceiverList.end() ) value = FALSE;
			else
			{
				PlayBackStreamReceiver *pbStreamReceiver;
				pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
				if( pbStreamReceiver )
				{
					Recorder *recorder = GetRecorder( chInfo );
					if( !recorder ) return FALSE;

					pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
					pbInfo.pbDirection = RS_PLAYBACK_DIRECTION_T_FORWARD;
					pbInfo.pbSpeed = RS_PLAYBACK_SPEED_T_1X;
					value = recorder->ControlPlay( &pbInfo );
				}
				else value = FALSE;
			}
		}
	}
	return value;
}

BOOL RecorderIF::ControlStop( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();
	for( index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}	
	return ControlStop( group->GetGroupUUID(), &chInfos );
}

BOOL RecorderIF::ControlStop( CString groupUUID, std::vector<CChannelInfo*> *chInfos )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		BOOL sValue = FALSE;
		CChannelInfo *chInfo;

		RS_PLAYBACK_INFO_T pbInfo = {0,};

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		chGrpRelatedRcrdIter = chGrpRelatedRcrdIterPair.first;
		if( chGrpRelatedRcrdIter==chGrpRelatedRcrdIterPair.second ) continue;
		chInfo = chGrpRelatedRcrdIter->second;
		if( chInfo )
		{
			CScopedLock lock( &_lockOfPbReceiver );
			std::map<CString,PBStreamReceiverList>::iterator iter;
			iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
			if( iter==_pbUUID.end() ) sValue = FALSE;
			else
			{
				PBStreamReceiverList pbStreamReceiverList = iter->second;
				PBStreamReceiverList::iterator iter2;
				iter2 = pbStreamReceiverList.find( groupUUID );
				if( iter2==pbStreamReceiverList.end() ) sValue = FALSE;
				else
				{
					PlayBackStreamReceiver *pbStreamReceiver;
					pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
					if( pbStreamReceiver )
					{
						Recorder *recorder = GetRecorder( chInfo );
						if( !recorder ) continue;

						pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
						sValue = recorder->ControlStop( &pbInfo );
					}
					else sValue = FALSE;
				}
			}
		}
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::ControlStop( CChannelInfo *chInfo )
{
	BOOL value = FALSE;
	RS_PLAYBACK_INFO_T pbInfo = {0,};
	if( chInfo )
	{
		CScopedLock lock( &_lockOfPbReceiver );
		std::map<CString,PBStreamReceiverList>::iterator iter;
		iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
		if( iter==_pbUUID.end() ) value = FALSE;
		else
		{
			PBStreamReceiverList pbStreamReceiverList = iter->second;
			PBStreamReceiverList::iterator iter2;
			iter2 = pbStreamReceiverList.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid );
			if( iter2==pbStreamReceiverList.end() ) value = FALSE;
			else
			{
				PlayBackStreamReceiver *pbStreamReceiver;
				pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
				if( pbStreamReceiver )
				{
					Recorder *recorder = GetRecorder( chInfo );
					if( !recorder ) return FALSE;

					pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
					value = recorder->ControlStop( &pbInfo );
				}
				else value = FALSE;
			}
		}
	}
	return value;
}

BOOL RecorderIF::ControlPause( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();
	for( index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}	
	return ControlPause( group->GetGroupUUID(), &chInfos );
}

BOOL RecorderIF::ControlPause( CString groupUUID, std::vector<CChannelInfo*> *chInfos )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		BOOL sValue = FALSE;
		CChannelInfo *chInfo;

		RS_PLAYBACK_INFO_T pbInfo = {0,};

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		chGrpRelatedRcrdIter = chGrpRelatedRcrdIterPair.first;
		if( chGrpRelatedRcrdIter==chGrpRelatedRcrdIterPair.second ) continue;
		chInfo = chGrpRelatedRcrdIter->second;
		if( chInfo )
		{
			CScopedLock lock( &_lockOfPbReceiver );
			std::map<CString,PBStreamReceiverList>::iterator iter;
			iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
			if( iter==_pbUUID.end() ) sValue = FALSE;
			else
			{
				PBStreamReceiverList pbStreamReceiverList = iter->second;
				PBStreamReceiverList::iterator iter2;
				iter2 = pbStreamReceiverList.find( groupUUID );
				if( iter2==pbStreamReceiverList.end() ) sValue = FALSE;
				else
				{
					PlayBackStreamReceiver *pbStreamReceiver;
					pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
					if( pbStreamReceiver )
					{
						Recorder *recorder = GetRecorder( chInfo );
						if( !recorder ) continue;

						pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
						sValue = recorder->ControlPause( &pbInfo );
					}
					else sValue = FALSE;
				}
			}
		}
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::ControlPause( CChannelInfo *chInfo )
{
	BOOL value = FALSE;
	RS_PLAYBACK_INFO_T pbInfo = {0,};

	if( chInfo )
	{
		CScopedLock lock( &_lockOfPbReceiver );
		std::map<CString,PBStreamReceiverList>::iterator iter;
		iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
		if( iter==_pbUUID.end() ) value = FALSE;
		else
		{
			PBStreamReceiverList pbStreamReceiverList = iter->second;
			PBStreamReceiverList::iterator iter2;
			iter2 = pbStreamReceiverList.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid );
			if( iter2==pbStreamReceiverList.end() ) value = FALSE;
			else
			{
				PlayBackStreamReceiver *pbStreamReceiver;
				pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
				if( pbStreamReceiver )
				{
					Recorder *recorder = GetRecorder( chInfo );
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

BOOL RecorderIF::ControlResume( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();
	for( index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}	
	return ControlResume( group->GetGroupUUID(), &chInfos );
}

BOOL RecorderIF::ControlResume( CString groupUUID, std::vector<CChannelInfo*> *chInfos )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		BOOL sValue = FALSE;
		CChannelInfo *chInfo;

		RS_PLAYBACK_INFO_T pbInfo = {0,};

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		chGrpRelatedRcrdIter = chGrpRelatedRcrdIterPair.first;
		if( chGrpRelatedRcrdIter==chGrpRelatedRcrdIterPair.second ) continue;
		chInfo = chGrpRelatedRcrdIter->second;
		if( chInfo )
		{
			CScopedLock lock( &_lockOfPbReceiver );
			std::map<CString,PBStreamReceiverList>::iterator iter;
			iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
			if( iter==_pbUUID.end() ) sValue = FALSE;
			else
			{
				PBStreamReceiverList pbStreamReceiverList = iter->second;
				PBStreamReceiverList::iterator iter2;
				iter2 = pbStreamReceiverList.find( groupUUID );
				if( iter2==pbStreamReceiverList.end() ) sValue = FALSE;
				else
				{
					PlayBackStreamReceiver *pbStreamReceiver;
					pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
					if( pbStreamReceiver )
					{
						Recorder *recorder = GetRecorder( chInfo );
						if( !recorder ) continue;

						pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
						sValue = recorder->ControlResume( &pbInfo );
					}
					else sValue = FALSE;
				}
			}
		}
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::ControlResume( CChannelInfo *chInfo )
{
	BOOL value = FALSE;
	RS_PLAYBACK_INFO_T pbInfo = {0,};
	if( chInfo )
	{
		CScopedLock lock( &_lockOfPbReceiver );
		std::map<CString,PBStreamReceiverList>::iterator iter;
		iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
		if( iter==_pbUUID.end() ) value = FALSE;
		else
		{
			PBStreamReceiverList pbStreamReceiverList = iter->second;
			PBStreamReceiverList::iterator iter2;
			iter2 = pbStreamReceiverList.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid );
			if( iter2==pbStreamReceiverList.end() ) value = FALSE;
			else
			{
				PlayBackStreamReceiver *pbStreamReceiver;
				pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
				if( pbStreamReceiver )
				{
					Recorder *recorder = GetRecorder( chInfo );
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

BOOL RecorderIF::ControlFowardPlay( CGroupInfo *group, UINT speed )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();
	for( index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}	
	return ControlFowardPlay( group->GetGroupUUID(), &chInfos, speed );
}

BOOL RecorderIF::ControlFowardPlay( CString groupUUID, std::vector<CChannelInfo*> *chInfos, UINT speed )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		BOOL sValue = FALSE;
		CChannelInfo *chInfo;

		RS_PLAYBACK_INFO_T pbInfo = {0,};

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		chGrpRelatedRcrdIter = chGrpRelatedRcrdIterPair.first;
		if( chGrpRelatedRcrdIter==chGrpRelatedRcrdIterPair.second ) continue;
		chInfo = chGrpRelatedRcrdIter->second;
		if( chInfo )
		{
			CScopedLock lock( &_lockOfPbReceiver );
			std::map<CString,PBStreamReceiverList>::iterator iter;
			iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
			if( iter==_pbUUID.end() ) sValue = FALSE;
			else
			{
				PBStreamReceiverList pbStreamReceiverList = iter->second;
				PBStreamReceiverList::iterator iter2;
				iter2 = pbStreamReceiverList.find( groupUUID );
				if( iter2==pbStreamReceiverList.end() ) sValue = FALSE;
				else
				{
					PlayBackStreamReceiver *pbStreamReceiver;
					pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
					if( pbStreamReceiver )
					{
						Recorder *recorder = GetRecorder( chInfo );
						if( !recorder ) continue;

						pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
						if( speed>6 ) speed = 6;
						pbInfo.pbSpeed = RS_PLAYBACK_SPEED( speed );
						sValue = recorder->ControlFowardPlay( &pbInfo );
					}
					else sValue = FALSE;
				}
			}
		}
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::ControlFowardPlay( CChannelInfo *chInfo, UINT speed )
{
	BOOL value = FALSE;
	RS_PLAYBACK_INFO_T pbInfo = {0,};

	if( chInfo )
	{
		CScopedLock lock( &_lockOfPbReceiver );
		std::map<CString,PBStreamReceiverList>::iterator iter;
		iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
		if( iter==_pbUUID.end() ) value = FALSE;
		else
		{
			PBStreamReceiverList pbStreamReceiverList = iter->second;
			PBStreamReceiverList::iterator iter2;
			iter2 = pbStreamReceiverList.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid );
			if( iter2==pbStreamReceiverList.end() ) value = FALSE;
			else
			{
				PlayBackStreamReceiver *pbStreamReceiver;
				pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
				if( pbStreamReceiver )
				{
					Recorder *recorder = GetRecorder( chInfo );
					if( !recorder ) return FALSE;

					pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
					if( speed>6 ) speed = 6;
					pbInfo.pbSpeed = RS_PLAYBACK_SPEED( speed );
					value = recorder->ControlFowardPlay( &pbInfo );
				}
				else value = FALSE;
			}
		}
	}
	return value;
}

BOOL RecorderIF::ControlBackwardPlay( CGroupInfo *group, UINT speed )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();
	for( index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}	
	return ControlBackwardPlay( group->GetGroupUUID(), &chInfos, speed );
}

BOOL RecorderIF::ControlBackwardPlay( CString groupUUID, std::vector<CChannelInfo*> *chInfos, UINT speed )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		BOOL sValue = FALSE;
		CChannelInfo *chInfo;

		RS_PLAYBACK_INFO_T pbInfo = {0,};

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		chGrpRelatedRcrdIter = chGrpRelatedRcrdIterPair.first;
		if( chGrpRelatedRcrdIter==chGrpRelatedRcrdIterPair.second ) continue;
		chInfo = chGrpRelatedRcrdIter->second;
		if( chInfo )
		{
			CScopedLock lock( &_lockOfPbReceiver );
			std::map<CString,PBStreamReceiverList>::iterator iter;
			iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
			if( iter==_pbUUID.end() ) sValue = FALSE;
			else
			{
				PBStreamReceiverList pbStreamReceiverList = iter->second;
				PBStreamReceiverList::iterator iter2;
				iter2 = pbStreamReceiverList.find( groupUUID );
				if( iter2==pbStreamReceiverList.end() ) sValue = FALSE;
				else
				{
					PlayBackStreamReceiver *pbStreamReceiver;
					pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
					if( pbStreamReceiver )
					{
						Recorder *recorder = GetRecorder( chInfo );
						if( !recorder ) continue;

						pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
						if( speed>6 ) speed = 6;
						pbInfo.pbSpeed = RS_PLAYBACK_SPEED( speed );
						sValue = recorder->ControlBackwardPlay( &pbInfo );
					}
					else sValue = FALSE;
				}
			}
		}
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::ControlBackwardPlay( CChannelInfo *chInfo, UINT speed )
{
	BOOL value = FALSE;
	RS_PLAYBACK_INFO_T pbInfo = {0,};
	if( chInfo )
	{
		CScopedLock lock( &_lockOfPbReceiver );
		std::map<CString,PBStreamReceiverList>::iterator iter;
		iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
		if( iter==_pbUUID.end() ) value = FALSE;
		else
		{
			PBStreamReceiverList pbStreamReceiverList = iter->second;
			PBStreamReceiverList::iterator iter2;
			iter2 = pbStreamReceiverList.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid );
			if( iter2==pbStreamReceiverList.end() ) value = FALSE;
			else
			{
				PlayBackStreamReceiver *pbStreamReceiver;
				pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
				if( pbStreamReceiver )
				{
					Recorder *recorder = GetRecorder( chInfo );
					if( !recorder ) return FALSE;

					pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
					if( speed>6 ) speed = 6;
					pbInfo.pbSpeed = RS_PLAYBACK_SPEED( speed );
					value = recorder->ControlBackwardPlay( &pbInfo );
				}
				else value = FALSE;
			}
		}
	}
	return value;
}

BOOL RecorderIF::ControlJump( CGroupInfo *group, UINT year, UINT month, UINT day, UINT hour, UINT minute, UINT second )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();
	for( index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}	
	return ControlJump( group->GetGroupUUID(), &chInfos, year, month, day, hour, minute, second );
}

BOOL RecorderIF::ControlJump( CString groupUUID, std::vector<CChannelInfo*> *chInfos, UINT year, UINT month, UINT day, UINT hour, UINT minute, UINT second )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		BOOL sValue = FALSE;
		CChannelInfo *chInfo;

		RS_PLAYBACK_JUMP_REQUEST_T pbJumpRequest = {0,};
		RS_PLAYBACK_INFO_T pbInfo = {0,};

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		chGrpRelatedRcrdIter = chGrpRelatedRcrdIterPair.first;
		if( chGrpRelatedRcrdIter==chGrpRelatedRcrdIterPair.second ) continue;
		chInfo = chGrpRelatedRcrdIter->second;
		if( chInfo )
		{
			CScopedLock lock( &_lockOfPbReceiver );
			std::map<CString,PBStreamReceiverList>::iterator iter;
			iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
			if( iter==_pbUUID.end() ) sValue = FALSE;
			else
			{
				PBStreamReceiverList pbStreamReceiverList = iter->second;
				PBStreamReceiverList::iterator iter2;
				iter2 = pbStreamReceiverList.find( groupUUID );
				if( iter2==pbStreamReceiverList.end() ) sValue = FALSE;
				else
				{
					PlayBackStreamReceiver *pbStreamReceiver;
					pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
					if( pbStreamReceiver )
					{
						Recorder *recorder = GetRecorder( chInfo );
						if( !recorder ) continue;

						pbJumpRequest.year = year;
						pbJumpRequest.month = month;
						pbJumpRequest.day = day;
						pbJumpRequest.hour = hour;
						pbJumpRequest.minute = minute;
						pbJumpRequest.second = second;

						pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
						sValue = recorder->ControlJump( &pbJumpRequest, &pbInfo );
					}
					else sValue = FALSE;
				}
			}
		}
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::ControlJump( CChannelInfo *chInfo, UINT year, UINT month, UINT day, UINT hour, UINT minute, UINT second )
{
	BOOL value = FALSE;
	RS_PLAYBACK_JUMP_REQUEST_T pbJumpRequest = {0,};
	RS_PLAYBACK_INFO_T pbInfo = {0,};

	if( chInfo )
	{
		CScopedLock lock( &_lockOfPbReceiver );
		std::map<CString,PBStreamReceiverList>::iterator iter;
		iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
		if( iter==_pbUUID.end() ) value = FALSE;
		else
		{
			PBStreamReceiverList pbStreamReceiverList = iter->second;
			PBStreamReceiverList::iterator iter2;
			iter2 = pbStreamReceiverList.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid );
			if( iter2==pbStreamReceiverList.end() ) value = FALSE;
			else
			{
				PlayBackStreamReceiver *pbStreamReceiver;
				pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
				if( pbStreamReceiver )
				{
					Recorder *recorder = GetRecorder( chInfo );
					if( !recorder ) return FALSE;

					pbJumpRequest.year = year;
					pbJumpRequest.month = month;
					pbJumpRequest.day = day;
					pbJumpRequest.hour = hour;
					pbJumpRequest.minute = minute;
					pbJumpRequest.second = second;

					pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
					value = recorder->ControlJump( &pbJumpRequest, &pbInfo );
				}
				else value = FALSE;
			}
		}
	}
	return value;
}

BOOL RecorderIF::ControlGoToFirst( CGroupInfo *group )//, UINT *year, UINT *month, UINT*day, UINT *hour, UINT *minute, UINT *second )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();
	for( index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}	
	return ControlGoToFirst( group->GetGroupUUID(), &chInfos );//, year, month, day, hour, minute, second );
}

BOOL RecorderIF::ControlGoToFirst( CString groupUUID, std::vector<CChannelInfo*> *chInfos)//, UINT *year, UINT *month, UINT *day, UINT *hour, UINT *minute, UINT *second )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}
	std::vector<CTime> timeList;
	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		BOOL sValue = FALSE;
		CChannelInfo *chInfo;

		RS_PLAYBACK_INFO_T pbInfo = {0,};
		RS_PLAYBACK_GOTO_FIRST_RESPONSE_T response = {0,};

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		chGrpRelatedRcrdIter = chGrpRelatedRcrdIterPair.first;
		if( chGrpRelatedRcrdIter==chGrpRelatedRcrdIterPair.second ) continue;
		chInfo = chGrpRelatedRcrdIter->second;
		if( chInfo )
		{
			CScopedLock lock( &_lockOfPbReceiver );
			std::map<CString,PBStreamReceiverList>::iterator iter;
			iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
			if( iter==_pbUUID.end() ) sValue = FALSE;
			else
			{
				PBStreamReceiverList pbStreamReceiverList = iter->second;
				PBStreamReceiverList::iterator iter2;
				iter2 = pbStreamReceiverList.find( groupUUID );
				if( iter2==pbStreamReceiverList.end() ) sValue = FALSE;
				else
				{
					PlayBackStreamReceiver *pbStreamReceiver;
					pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
					if( pbStreamReceiver )
					{
						Recorder *recorder = GetRecorder( chInfo );
						if( !recorder ) continue;

						pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
						sValue = recorder->ControlGoToFirst( &pbInfo, &response );
						if( sValue )
						{
							CTime time( int(response.year), int(response.month), int(response.day), int(response.hour), int(response.minute), int(response.second) );
							timeList.push_back( time );
						}
					}
					else sValue = FALSE;
				}
			}
		}
		value &= sValue;
	}
	return value;
}

BOOL  RecorderIF::ControlGoToFirst( CChannelInfo *chInfo )
{
	BOOL value = FALSE;
	RS_PLAYBACK_INFO_T pbInfo = {0,};
	RS_PLAYBACK_GOTO_FIRST_RESPONSE_T response = {0,};

	if( chInfo )
	{
		CScopedLock lock( &_lockOfPbReceiver );
		std::map<CString,PBStreamReceiverList>::iterator iter;
		iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
		if( iter==_pbUUID.end() ) value = FALSE;
		else
		{
			PBStreamReceiverList pbStreamReceiverList = iter->second;
			PBStreamReceiverList::iterator iter2;
			iter2 = pbStreamReceiverList.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid );
			if( iter2==pbStreamReceiverList.end() ) value = FALSE;
			else
			{
				PlayBackStreamReceiver *pbStreamReceiver;
				pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
				if( pbStreamReceiver )
				{
					Recorder *recorder = GetRecorder( chInfo );
					if( !recorder ) return FALSE;

					pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();

					value = recorder->ControlGoToFirst( &pbInfo, &response );
				}
				else value = FALSE;
			}
		}
	}
	return value;
}

BOOL RecorderIF::ControlGoToLast( CGroupInfo *group )//, UINT *year, UINT *month, UINT*day, UINT *hour, UINT *minute, UINT *second )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();
	for( index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}
	}	
	return ControlGoToLast( group->GetGroupUUID(), &chInfos );//, year, month, day, hour, minute, second );
}

BOOL RecorderIF::ControlGoToLast( CString groupUUID, std::vector<CChannelInfo*> *chInfos )//, UINT *year, UINT *month, UINT *day, UINT *hour, UINT *minute, UINT *second )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	std::vector<CTime> timeList;
	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		BOOL sValue = FALSE;
		CChannelInfo *chInfo;

		RS_PLAYBACK_INFO_T pbInfo = {0,};
		RS_PLAYBACK_GOTO_LAST_RESPONSE_T response = {0,};

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		chGrpRelatedRcrdIter = chGrpRelatedRcrdIterPair.first;
		if( chGrpRelatedRcrdIter==chGrpRelatedRcrdIterPair.second ) continue;
		chInfo = chGrpRelatedRcrdIter->second;
		if( chInfo )
		{
			CScopedLock lock( &_lockOfPbReceiver );
			std::map<CString,PBStreamReceiverList>::iterator iter;
			iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
			if( iter==_pbUUID.end() ) sValue = FALSE;
			else
			{
				PBStreamReceiverList pbStreamReceiverList = iter->second;
				PBStreamReceiverList::iterator iter2;
				iter2 = pbStreamReceiverList.find( groupUUID );
				if( iter2==pbStreamReceiverList.end() ) sValue = FALSE;
				else
				{
					PlayBackStreamReceiver *pbStreamReceiver;
					pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
					if( pbStreamReceiver )
					{
						Recorder *recorder = GetRecorder( chInfo );
						if( !recorder ) continue;

						pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
						sValue = recorder->ControlGoToLast( &pbInfo, &response );
						if( sValue )
						{
							CTime time( response.year, response.month, response.day, response.hour, response.minute, response.second );
							timeList.push_back( time );
						}
					}
					else sValue = FALSE;
				}
			}
		}
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::ControlGoToLast( CChannelInfo *chInfo )
{
	BOOL value = FALSE;
	RS_PLAYBACK_INFO_T pbInfo = {0,};
	RS_PLAYBACK_GOTO_LAST_RESPONSE_T response = {0,};

	if( chInfo )
	{
		CScopedLock lock( &_lockOfPbReceiver );
		std::map<CString,PBStreamReceiverList>::iterator iter;
		iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
		if( iter==_pbUUID.end() ) value = FALSE;
		else
		{
			PBStreamReceiverList pbStreamReceiverList = iter->second;
			PBStreamReceiverList::iterator iter2;
			iter2 = pbStreamReceiverList.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid );
			if( iter2==pbStreamReceiverList.end() ) value = FALSE;
			else
			{
				PlayBackStreamReceiver *pbStreamReceiver;
				pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
				if( pbStreamReceiver )
				{
					Recorder *recorder = GetRecorder( chInfo );
					if( !recorder ) return FALSE;

					pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
					value = recorder->ControlGoToLast( &pbInfo, &response );
				}
				else value = FALSE;
			}
		}
	}
	return value;
}

BOOL RecorderIF::ControlForwardStep( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();
	for( index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}

	}	
	return ControlForwardStep( group->GetGroupUUID(), &chInfos );//, year, month, day, hour, minute, second );
}

BOOL RecorderIF::ControlForwardStep( CString groupUUID, std::vector<CChannelInfo*> *chInfos )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	std::vector<CTime> timeList;
	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		BOOL sValue = FALSE;
		CChannelInfo *chInfo;

		RS_PLAYBACK_INFO_T pbInfo = {0,};

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		chGrpRelatedRcrdIter = chGrpRelatedRcrdIterPair.first;
		if( chGrpRelatedRcrdIter==chGrpRelatedRcrdIterPair.second ) continue;
		chInfo = chGrpRelatedRcrdIter->second;
		if( chInfo )
		{
			CScopedLock lock( &_lockOfPbReceiver );
			std::map<CString,PBStreamReceiverList>::iterator iter;
			iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
			if( iter==_pbUUID.end() ) sValue = FALSE;
			else
			{
				PBStreamReceiverList pbStreamReceiverList = iter->second;
				PBStreamReceiverList::iterator iter2;
				iter2 = pbStreamReceiverList.find( groupUUID );
				if( iter2==pbStreamReceiverList.end() ) sValue = FALSE;
				else
				{
					PlayBackStreamReceiver *pbStreamReceiver;
					pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
					if( pbStreamReceiver )
					{
						Recorder *recorder = GetRecorder( chInfo );
						if( !recorder ) continue;

						pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
						sValue = recorder->ControlForwardStep( &pbInfo );
					}
					else sValue = FALSE;
				}
			}
		}
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::ControlForwardStep( CChannelInfo *chInfo )
{
	BOOL value = FALSE;
	RS_PLAYBACK_INFO_T pbInfo = {0,};

	if( chInfo )
	{
		CScopedLock lock( &_lockOfPbReceiver );
		std::map<CString,PBStreamReceiverList>::iterator iter;
		iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
		if( iter==_pbUUID.end() ) value = FALSE;
		else
		{
			PBStreamReceiverList pbStreamReceiverList = iter->second;
			PBStreamReceiverList::iterator iter2;
			iter2 = pbStreamReceiverList.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid );
			if( iter2==pbStreamReceiverList.end() ) value = FALSE;
			else
			{
				PlayBackStreamReceiver *pbStreamReceiver;
				pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
				if( pbStreamReceiver )
				{
					Recorder *recorder = GetRecorder( chInfo );
					if( !recorder ) return FALSE;

					pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
					value = recorder->ControlForwardStep( &pbInfo );
				}
				else value = FALSE;
			}
		}
	}
	return value;
}

BOOL RecorderIF::ControlBackwardStep( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();
	for( index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		if(camInfo)
		{
			chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
			if(chInfo) chInfos.push_back( chInfo );
		}

	}	
	return ControlBackwardStep( group->GetGroupUUID(), &chInfos );//, year, month, day, hour, minute, second );
}

BOOL RecorderIF::ControlBackwardStep( CString groupUUID, std::vector<CChannelInfo*> *chInfos )
{
	BOOL value = TRUE;
	std::multimap<CString,CChannelInfo*> chGrpRelatedRcrd;
	std::multimap<CString,CChannelInfo*>::iterator chGrpRelatedRcrdIter;
	std::pair<std::multimap<CString,CChannelInfo*>::iterator, std::multimap<CString,CChannelInfo*>::iterator> chGrpRelatedRcrdIterPair;

	std::vector<CChannelInfo*>::iterator iter;

	for( iter=chInfos->begin(); iter!=chInfos->end(); iter++ )
	{
		chGrpRelatedRcrd.insert( std::pair<CString,CChannelInfo*>((*iter)->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, (*iter)) );
	}

	std::vector<CString> keyList;
	std::vector<CString>::iterator keyIter;
	for( chGrpRelatedRcrdIter=chGrpRelatedRcrd.begin(); 
		 chGrpRelatedRcrdIter!=chGrpRelatedRcrd.end(); 
		 chGrpRelatedRcrdIter=chGrpRelatedRcrd.upper_bound(chGrpRelatedRcrdIter->first) )
	{
		keyList.push_back( chGrpRelatedRcrdIter->first );
	}

	std::vector<CTime> timeList;
	for( keyIter=keyList.begin(); keyIter!=keyList.end(); keyIter++ )
	{
		BOOL sValue = FALSE;
		CChannelInfo *chInfo;

		RS_PLAYBACK_INFO_T pbInfo = {0,};

		chGrpRelatedRcrdIterPair = chGrpRelatedRcrd.equal_range( (*keyIter) );
		chGrpRelatedRcrdIter = chGrpRelatedRcrdIterPair.first;
		if( chGrpRelatedRcrdIter==chGrpRelatedRcrdIterPair.second ) continue;
		chInfo = chGrpRelatedRcrdIter->second;
		if( chInfo )
		{
			CScopedLock lock( &_lockOfPbReceiver );
			std::map<CString,PBStreamReceiverList>::iterator iter;
			iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
			if( iter==_pbUUID.end() ) sValue = FALSE;
			else
			{
				PBStreamReceiverList pbStreamReceiverList = iter->second;
				PBStreamReceiverList::iterator iter2;
				iter2 = pbStreamReceiverList.find( groupUUID );
				if( iter2==pbStreamReceiverList.end() ) sValue = FALSE;
				else
				{
					PlayBackStreamReceiver *pbStreamReceiver;
					pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
					if( pbStreamReceiver )
					{
						Recorder *recorder = GetRecorder( chInfo );
						if( !recorder ) continue;

						pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
						sValue = recorder->ControlBackwardStep( &pbInfo );
					}
					else sValue = FALSE;
				}
			}
		}
		value &= sValue;
	}
	return value;
}

BOOL RecorderIF::ControlBackwardStep( CChannelInfo *chInfo )
{
	BOOL value = FALSE;
	RS_PLAYBACK_INFO_T pbInfo = {0,};

	if( chInfo )
	{
		CScopedLock lock( &_lockOfPbReceiver );
		std::map<CString,PBStreamReceiverList>::iterator iter;
		iter = _pbUUID.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid );
		if( iter==_pbUUID.end() ) value = FALSE;
		else
		{
			PBStreamReceiverList pbStreamReceiverList = iter->second;
			PBStreamReceiverList::iterator iter2;
			iter2 = pbStreamReceiverList.find( chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid );
			if( iter2==pbStreamReceiverList.end() ) value = FALSE;
			else
			{
				PlayBackStreamReceiver *pbStreamReceiver;
				pbStreamReceiver = static_cast<PlayBackStreamReceiver*>( iter2->second );
				if( pbStreamReceiver )
				{
					Recorder *recorder = GetRecorder( chInfo );
					if( !recorder ) return FALSE;

					pbInfo.playbackID = pbStreamReceiver->GetPlaybackID();
					value = recorder->ControlBackwardStep( &pbInfo );
				}
				else value = FALSE;
			}
		}
	}
	return value;
}

/*
BOOL RecorderIF::StartExport( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();
	for( index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
		chInfos.push_back( chInfo );
	}	
	return StartExport( group->GetGroupUUID(), &chInfos );
}

BOOL RecorderIF::StartExport( CString groupUUID, std::vector<CChannelInfo*> *chInfos )
{

	return FALSE;
}
*/

BOOL RecorderIF::StartExport( CChannelInfo *chInfo, CString szFilePath, 
	UINT sYear, UINT sMonth, UINT sDay, UINT sHour, UINT sMinute, 
	UINT eYear, UINT eMonth, UINT eDay, UINT eHour, UINT eMinute, 
	LPEXPORTMSGHANDLER pHandler, LPVOID pParam)
{
	Recorder *recorder	= GetRecorder( chInfo );
	if( !recorder ) return FALSE;
	BOOL value = FALSE;
	if( chInfo )
	{
		CScopedLock lock( &_lockOfExpReceiver );
		if( !_exportReceiver )
		{
			RS_DEVICE_INFO_SET_T *devInfoList = new RS_DEVICE_INFO_SET_T;
			RS_EXPORT_REQUEST_T expReq;
			RS_EXPORT_RESPONSE_T expRes;
			devInfoList->validDeviceCount = 1;
			if( !MakeDeviceInfo(chInfo, &(devInfoList->deviceInfo[0])) ) 
			{
				if( devInfoList ) delete devInfoList;
				return FALSE;
			}

			_exportReceiver	= new ExportStreamReceiver();
			_exportReceiver->RegisterMsgHandler(pHandler, pParam);

			CTime sTime( sYear, sMonth, sDay, sHour, sMinute, 0 );
			CTime eTime( eYear, eMonth, eDay, eHour, eMinute, 0 );
			_exportReceiver->Start( sTime, eTime, szFilePath );
			expReq.sYear = sYear;
			expReq.sMonth = sMonth;
			expReq.sDay = sDay;
			expReq.sHour = sHour;
			expReq.sMinute = sMinute;
			expReq.sSecond = 0;
			expReq.eYear = eYear;
			expReq.eMonth = eMonth;
			expReq.eDay = eDay;
			expReq.eHour = eHour;
			expReq.eMinute = eMinute;
			expReq.eSecond = 0;
			expReq.pReceiver = _exportReceiver;
			expReq.type = RS_EP_FRAME_ALL;//RS_EP_FRAME_ALL;//RS_EP_FRAME_VIDEO;
			value = recorder->StartExport( devInfoList, &expReq, &expRes );

			if( devInfoList ) delete devInfoList;
		}
		else
		{
			RS_EXPORT_INFO_T epInfo = {0,};
			epInfo.exportID = _exportReceiver->GetExportID();
			Recorder *recorder	= GetRecorder( chInfo );
			value = recorder->StopExport( &epInfo );

			RS_DEVICE_INFO_SET_T *devInfoList = new RS_DEVICE_INFO_SET_T;
			RS_EXPORT_REQUEST_T expReq;
			RS_EXPORT_RESPONSE_T expRes;
			devInfoList->validDeviceCount = 1;
			if( !MakeDeviceInfo(chInfo, &(devInfoList->deviceInfo[0])) ) 
			{
				if( devInfoList ) delete devInfoList;
				return FALSE;
			}

			_exportReceiver	= new ExportStreamReceiver();
			_exportReceiver->RegisterMsgHandler(pHandler, pParam);

			CTime sTime( sYear, sMonth, sDay, sHour, sMinute, 0 );
			CTime eTime( eYear, eMonth, eDay, eHour, eMinute, 0 );
			_exportReceiver->Start( sTime, eTime, szFilePath );
			expReq.sYear = sYear;
			expReq.sMonth = sMonth;
			expReq.sDay = sDay;
			expReq.sHour = sHour;
			expReq.sMinute = sMinute;
			expReq.sSecond = 0;
			expReq.eYear = eYear;
			expReq.eMonth = eMonth;
			expReq.eDay = eDay;
			expReq.eHour = eHour;
			expReq.eMinute = eMinute;
			expReq.eSecond = 0;
			expReq.pReceiver = _exportReceiver;
			expReq.type = RS_EP_FRAME_ALL;
			value = recorder->StartExport( devInfoList, &expReq, &expRes );

			if( devInfoList ) delete devInfoList;
		}
	}
	return value;
}

/*
BOOL RecorderIF::StopExport( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();
	for( index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
		chInfos.push_back( chInfo );
	}	
	return StopExport( group->GetGroupUUID(), &chInfos );
}

BOOL RecorderIF::StopExport( CString groupUUID, std::vector<CChannelInfo*> *chInfos )
{

	return FALSE;
}
*/

BOOL RecorderIF::StopExport( CChannelInfo *chInfo )
{
	BOOL value = FALSE;
	RS_EXPORT_INFO_T epInfo = {0,};
	if( chInfo )
	{
		CScopedLock lock( &_lockOfExpReceiver );
		if( _exportReceiver )
		{
			_exportReceiver->Stop();
			epInfo.exportID = _exportReceiver->GetExportID();
			Recorder *recorder	= GetRecorder( chInfo );
			value = recorder->StopExport( &epInfo );

			//do { ::Sleep( 10 ); } while( _exportReceiver->OnReceive );
			delete _exportReceiver;
			_exportReceiver = NULL;
		}
		else
		{
			return FALSE;
		}
	}
	return value;
}

/*
BOOL RecorderIF::PauseExport( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();
	for( index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
		chInfos.push_back( chInfo );
	}	
	return PauseExport( group->GetGroupUUID(), &chInfos );
}

BOOL RecorderIF::PauseExport( CString groupUUID, std::vector<CChannelInfo*> *chInfos )
{

	return FALSE;
}

BOOL RecorderIF::PauseExport( CChannelInfo *chInfo )
{

	return FALSE;
}

BOOL RecorderIF::ResumeExport( CGroupInfo *group )
{
	std::vector<CChannelInfo*> chInfos;
	CCameraInfo *camInfo;
	CChannelInfo *chInfo;
	UINT16 index = 0;
	UINT16 count = group->GetCount();
	for( index=0; index<count; index++ )
	{
		camInfo = group->GetByIndex( index );
		chInfo = gChannelManager->GetChannelInfo( camInfo->GetUUID() );
		chInfos.push_back( chInfo );
	}	
	return ResumeExport( group->GetGroupUUID(), &chInfos );
}

BOOL RecorderIF::ResumeExport( CString groupUUID, std::vector<CChannelInfo*> *chInfos )
{

	return FALSE;
}

BOOL RecorderIF::ResumeExport( CChannelInfo *chInfo )
{

	return FALSE;
}
*/

Recorder * RecorderIF::GetRecorder( CChannelInfo *chInfo, UINT nRetryCount )
{
	RS_SERVER_INFO_T serverInfo;
	serverInfo.strServerId = chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid;
	serverInfo.strAddress = chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdIP;
	serverInfo.strUserId = chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdAccessID;
	serverInfo.strUserPassword = chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdAccessPW;
	return GetRecorder( &serverInfo, nRetryCount );
}

Recorder * RecorderIF::GetRecorder( RS_SERVER_INFO_T *serverInfo, UINT nRetryCount )
{
	if(serverInfo->strAddress.GetLength()==0)
		return NULL;

	std::map<CString,Recorder*>::iterator iter;
	BOOL bRetry = FALSE;
	if( nRetryCount>0 ) bRetry = TRUE;

	CScopedLock lock( &_lockOfRecorder );
	iter = _recorderList.find( serverInfo->strServerId );
	if( iter==_recorderList.end() )
	{
		Recorder* recorder = new Recorder( _notifier );
		BOOL isConnected = recorder->IsConnected();
		if( !isConnected )
		{
			isConnected = recorder->Connect( (LPTSTR)(LPCTSTR)serverInfo->strServerId, _bRunAsRecorder, (LPTSTR)(LPCTSTR)serverInfo->strAddress, (LPTSTR)(LPCTSTR)serverInfo->strUserId, (LPTSTR)(LPCTSTR)serverInfo->strUserPassword );
			/*
			XSleep( 10 );
			if( bRetry ) 
			{ 
				nRetryCount--; 
				//if( nRetryCount==0 ) return FALSE;
			}
			*/
		}
		if( isConnected )
		{
			_recorderList.insert( std::make_pair(serverInfo->strServerId, recorder) );
			return recorder;
		}
		else
		{
			CString szMessage = _T("");
#ifdef LANGUAGEPACK
			szMessage.Format(_T("Recorder[%s] %s"), serverInfo->strAddress, g_LanguageMapClass->Get(g_nLanguageType, L"S171"));
#else
			szMessage.Format( _T("Recorder[%s] 연결에 실패하였습니다."), serverInfo->strAddress );
#endif
			g_mainDlg->SendLog( szMessage );
			delete recorder;
			return NULL; 
		}
	}
	else 
	{
		Recorder* recorder = (*iter).second;
		BOOL isConnected = recorder->IsConnected();
		if( !isConnected )
		{
			isConnected = recorder->Connect( (LPTSTR)(LPCTSTR)serverInfo->strServerId, _bRunAsRecorder, (LPTSTR)(LPCTSTR)serverInfo->strAddress, (LPTSTR)(LPCTSTR)serverInfo->strUserId, (LPTSTR)(LPCTSTR)serverInfo->strUserPassword );
			/*
			XSleep( 10 );
			if( bRetry ) 
			{ 
				nRetryCount--; 
				//if( nRetryCount==0 ) return FALSE;
			}
			*/
		}
		if( isConnected ) return recorder;
		else
		{
			CString szMessage = _T("");
			szMessage.Format( _T("Recorder[%s]의 최대 접속자 수가 초과되었습니다."), serverInfo->strAddress );

			g_mainDlg->SendLog( szMessage );
			delete recorder;
			_recorderList.erase(iter);
			return NULL;
		}

	}
}

VOID RecorderIF::GenerateQuasiMac( CString *vcamMacAddress )
{
	int i,tp;
	(*vcamMacAddress) = "";
	::Sleep(1);
	DWORD tick = GetTickCount();
	int pid = getpid();
	srand( UINT(tick)+UINT(pid) );
	for (i=0; i<6 && (tp=rand()%32) > -1; i++) 
	{
		TCHAR macToken[5] = {0};
		::Sleep(1);
#if defined(UNICODE)
		_snwprintf( macToken, sizeof(macToken), _T("%s%X"),  tp<16 ? "0" : "", tp );
#else
		_snprintf( macToken, sizeof(macToken), _T("%s%X"),  tp<16 ? "0" : "", tp );
#endif
		(*vcamMacAddress).Append( macToken );
	}
}
	
UINT16 RecorderIF::GetIndexOfMaxResolution( VCAM_STREAM_URL *vcamStreamUrl )
{
	UINT32 resolution = 0;
	UINT16 selectedIndex = 0;
	for( UINT16 index=0; index<vcamStreamUrl->sizevcamlivestreamurl; index++ )
	{
		UINT32 curResolution = vcamStreamUrl->livestream[index].height*vcamStreamUrl->livestream[index].width;
		if( curResolution>resolution )
		{
			resolution = curResolution;
			selectedIndex = index;
		}
	}
	return selectedIndex;
}

VOID RecorderIF::GetDeviceUrls( CString *szVcamStreamUrl, CString *szIpAddress, CString *szProfileName, UINT16 &nRtspPort )
{
	if( szVcamStreamUrl==NULL ) return;
	CString vcamRtspPort;
	//vcamStreamUrl = "rtsp://192.168.40.30:1554/112_520p";
	INT32 frstIdxOfIP = (CString(L"rtsp://")).GetLength();
	INT32 lstIdxOfIP = 0;

	INT32 frstIdxOfRtspPort = szVcamStreamUrl->ReverseFind( ':' );
	lstIdxOfIP = frstIdxOfRtspPort;

	INT32 lstIdxOfRtspPort = szVcamStreamUrl->Find( L"554/" );
	frstIdxOfRtspPort++;
	lstIdxOfRtspPort += 3;

	INT32 frstIdxOfSlsh = szVcamStreamUrl->ReverseFind( '/' );

	(*szIpAddress) = szVcamStreamUrl->Mid( frstIdxOfIP, lstIdxOfIP-frstIdxOfIP );
	(*szProfileName) = szVcamStreamUrl->Mid( frstIdxOfSlsh );
	vcamRtspPort = szVcamStreamUrl->Mid( frstIdxOfRtspPort, lstIdxOfRtspPort-frstIdxOfRtspPort );
	nRtspPort = _ttoi( vcamRtspPort );
}

BOOL RecorderIF::MakeDeviceInfo( CChannelInfo *chInfo, RS_DEVICE_INFO_T *devInfo )
{
	if( chInfo==NULL ) return FALSE;
	if( chInfo->m_vamInfoEx==NULL ) return FALSE;
	if( wcslen(chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid)<1 ) return FALSE;
	if( wcslen(chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid)<1 ) return FALSE;
	if( chInfo->m_vamInfoEx->m_vcamInfo.vcamLivestreamInfo.sizevcamlivestreamurl<1 ) return FALSE;
	if( wcslen(chInfo->m_vamInfoEx->m_vcamInfo.camModelName)<1 ) return FALSE;

	devInfo->SetDeviceType( RS_DEVICE_ONVIF_CAMERA );
	devInfo->SetID( (CStringW)chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid );

	// Added vcamMacAddress
	if( wcslen(chInfo->m_vamInfoEx->m_vcamInfo.vcamMacAddress)>0 )
	{
		devInfo->SetMAC( chInfo->m_vamInfoEx->m_vcamInfo.vcamMacAddress );
	}
	else
	{
		CString szVcamMacAddress;
		GenerateQuasiMac( &szVcamMacAddress );
		devInfo->SetMAC( szVcamMacAddress );
	}

//  Recorder에서 임의로 Camera MacAddress 생성
//	CString szVcamMacAddress;
//	GenerateQuasiMac( &szVcamMacAddress );
//	devInfo->SetMAC( szVcamMacAddress );

	//UINT16 index = GetIndexOfMaxResolution( &(chInfo->m_vamInfoEx->m_vcamInfo.vcamLivestreamInfo) );
	//CString szVcamStreamUrl = chInfo->m_vamInfoEx->m_vcamInfo.vcamLivestreamInfo.livestream[index].url;
	UINT16 index=chInfo->m_vamInfoEx->m_vcamInfo.vcamLivestreamInfo.sizevcamlivestreamurl-1;
	CString szVcamStreamUrl;
	if(StrCmpW(chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdLiveStreamUrl, L"")==0)
		szVcamStreamUrl  = chInfo->m_vamInfoEx->m_vcamInfo.vcamLivestreamInfo.livestream[index].url;
	else
		szVcamStreamUrl = chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdLiveStreamUrl;

	CString szVcamIpOfDistributor;
	CString szVcamProfile;
	UINT16 nRtspPort = 0;
	GetDeviceUrls( &szVcamStreamUrl, &szVcamIpOfDistributor, &szVcamProfile, nRtspPort );

	devInfo->SetProfileName( szVcamProfile );
	devInfo->SetModelType( _T("ONVIF") );
	devInfo->SetModel( chInfo->m_vamInfoEx->m_vcamInfo.camModelName );
	devInfo->SetAddress( szVcamIpOfDistributor );
	devInfo->SetURL( szVcamStreamUrl );
	devInfo->SetRTSPPort( nRtspPort );
	devInfo->SetHTTPPort( 80 );
	devInfo->SetHTTPSPort( 443 );
	devInfo->SetSSL( FALSE );
	devInfo->SetConnectionType( RS_DEVICE_CONNECT_T_RTPoverRTSP );

	TCHAR onvifServiceUri[64] =  {0};
	CString szOnvifServiceUri;
	szOnvifServiceUri.Format( _T("http://%s/onvif/device_services"), szVcamIpOfDistributor );
	devInfo->SetOnvifServiceUri( szOnvifServiceUri );

	devInfo->SetName( chInfo->m_vamInfoEx->m_vcamInfo.vcamLocalName );
	devInfo->SetUser( chInfo->m_vamInfoEx->m_vcamInfo.camAdminId );
	devInfo->SetPassword( chInfo->m_vamInfoEx->m_vcamInfo.camAdminPw );

	return TRUE;
}

BOOL RecorderIF::MakeSchedInfo( CChannelInfo *chInfo, RS_RECORD_SCHEDULE_INFO_T *schedInfo, UINT schedType, BOOL audio, 
							    UINT bitSun, UINT bitMon, UINT bitTue, UINT bitWed, UINT bitThu, UINT bitFri, UINT bitSat, 			
								UINT preRecordingTime, UINT postRecordingTime )
{
	schedInfo->strStreamID = chInfo->m_vamInfoEx->m_vcamInfo.vcamUuid;
	schedInfo->nPre = preRecordingTime;
	schedInfo->nPost = postRecordingTime;

	//TO DO
	schedInfo->nRecordingMode = RS_RECORDING_MODE_T_NORMAL;
	if( schedInfo->nRecordingMode==RS_RECORDING_MODE_T_NORMAL )
	{
		schedInfo->validNormalSchedCount = 1;
		for( UINT index=0; index<schedInfo->validNormalSchedCount; index++ )
		{
			if( schedType<UINT(RS_RECORD_SCHEDULE_T_ALWAYS) || schedType>UINT(RS_RECORD_SCHEDULE_T_ADAPTIVE) )
				schedInfo->normalSchedInfo[index].nScheduleType	= RS_RECORD_SCHEDULE_T_ALWAYS;
			else 
				schedInfo->normalSchedInfo[index].nScheduleType = RS_RECORD_SCHEDULE(schedType);
			//schedInfo->normalSchedInfo[index].nScheduleType	= RS_RECORD_SCHEDULE_T_ALWAYS;//RS_RECORD_SCHEDULE(schedType);

			if( audio )
				schedInfo->normalSchedInfo[index].nAudio = 1;
			else
				schedInfo->normalSchedInfo[index].nAudio = 0;

			schedInfo->normalSchedInfo[index].nSun_bitflag	= bitSun;
			schedInfo->normalSchedInfo[index].nMon_bitflag	= bitMon;
			schedInfo->normalSchedInfo[index].nTue_bitflag	= bitTue;
			schedInfo->normalSchedInfo[index].nWed_bitflag	= bitWed;
			schedInfo->normalSchedInfo[index].nThu_bitflag	= bitThu;
			schedInfo->normalSchedInfo[index].nFri_bitflag	= bitFri;
			schedInfo->normalSchedInfo[index].nSat_bitflag	= bitSat;
		}
	}
	else if( schedInfo->nRecordingMode==RS_RECORDING_MODE_T_SPECIAL )
	{
		

	}
	return TRUE;
}

#endif