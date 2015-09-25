// VMSRecorderService.cpp : 해당 DLL의 초기화 루틴을 정의합니다.
//

#include "Common.h"
#include <XML.h>
#include <MessageSender.h>
#include <string_helper.h>
#include <ScopeLock.h>
#include "ServiceCoordinator.h"
#include "atlenc.h"

#include "VMSRecorderService.h"

VOID CALLBACK OnConnectionStopCallBack( CONST VOID* pData, size_t nDataSize, VOID* pUserContext )
{
	ServiceCore *svcCore = static_cast<ServiceCore*>( pUserContext );
	if( !svcCore ) return;
	CHAR *pXML = static_cast<CHAR*>( const_cast<VOID*>(pData) );
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) ) return;
	MSXML2::IXMLDOMNodePtr pNode = xml.FindNode(_T("//NRSError"));
	if(pNode)
	{
		RS_CONNECTION_STOP_NOTIFICATION_T notification;
		notification.strReason = CXML::GetText( pNode );
		if( notification.strReason.GetLength()>0 )
		{
			svcCore->OnConnectionStop( &notification );
		}
	}
}

VOID CALLBACK OnRecordingStorageFullCallBack( CONST VOID* pData, size_t nDataSize, VOID* pUserContext )
{
	ServiceCore *svcCore = static_cast<ServiceCore*>( pUserContext );
	if( !svcCore ) return;
	CHAR *pXML = static_cast<CHAR*>( const_cast<VOID*>(pData) );
	CXML xml;

	if( !xml.LoadXMLFromString(pXML) ) return;

	RS_STORAGE_FULL_NOTIFICATION_T notification;
	MSXML2::IXMLDOMNodeListPtr pNodeList = xml.FindNodeList(_T("//Drive"));
	if( pNodeList ) 
	{
		long count = pNodeList->Getlength();
		notification.validDeviceAndPathCount = UINT( count );
		for(long i=0; i<count; i++)
		{
			MSXML2::IXMLDOMNodePtr pNode = pNodeList->Getitem(i);
			notification.driveAndPath[i].strRecordingPath = CXML::GetText( pNode );
			notification.driveAndPath[i].strVolumeSerial = CXML::GetAttributeValue( pNode, L"volume_serial" );
		}
	}

	pNodeList = xml.FindNodeList(_T("//MAC"));
	if( pNodeList )
	{
		long count = pNodeList->Getlength();
		notification.validMacCount = UINT( count );
		for(long i=0; i<count; i++)
		{
			MSXML2::IXMLDOMNodePtr pNode = pNodeList->Getitem(i);
			notification.strMac[i] = CXML::GetText( pNode );
		}
	}

	if( (notification.validDeviceAndPathCount>0) || (notification.validMacCount>0) ) 
	{
		svcCore->OnRecordingStorageFull( &notification );
	}
}

VOID CALLBACK OnReservedStorageFullCallBack( CONST VOID* pData, size_t nDataSize, VOID* pUserContext )
{
	ServiceCore *svcCore = static_cast<ServiceCore*>( pUserContext );
	if( !svcCore ) return;
	CHAR *pXML = static_cast<CHAR*>( const_cast<VOID*>(pData) );
	CXML xml;

	if( !xml.LoadXMLFromString(pXML) ) return;
	RS_STORAGE_FULL_NOTIFICATION_T notification;
	MSXML2::IXMLDOMNodeListPtr pNodeList = xml.FindNodeList(_T("//Drive"));
	if( pNodeList ) 
	{
		long count = pNodeList->Getlength();
		notification.validDeviceAndPathCount = UINT( count );
		for(long i=0; i<count; i++)
		{
			MSXML2::IXMLDOMNodePtr pNode = pNodeList->Getitem(i);
			notification.driveAndPath[i].strRecordingPath = CXML::GetText( pNode );
			notification.driveAndPath[i].strVolumeSerial = CXML::GetAttributeValue( pNode, L"volume_serial" );
		}
	}

	pNodeList = xml.FindNodeList(_T("//MAC"));
	if( pNodeList )
	{
		long count = pNodeList->Getlength();
		notification.validMacCount = UINT( count );
		for(long i=0; i<count; i++)
		{
			MSXML2::IXMLDOMNodePtr pNode = pNodeList->Getitem(i);
			notification.strMac[i] = CXML::GetText( pNode );
		}
	}

	if( (notification.validDeviceAndPathCount>0) || (notification.validMacCount>0) ) 
	{
		svcCore->OnReservedStorageFull( &notification );
	}
}

VOID CALLBACK OnOverwritingErrorCallBack( CONST VOID* pData, size_t nDataSize, VOID* pUserContext )
{
	ServiceCore *svcCore = static_cast<ServiceCore*>( pUserContext );
	if( !svcCore ) return;
	CHAR *pXML = static_cast<CHAR*>( const_cast<VOID*>(pData) );
	CXML xml;

	if( !xml.LoadXMLFromString(pXML) ) return;
	MSXML2::IXMLDOMNodeListPtr pNodeList = xml.FindNodeList(_T("//TargetFilePath"));
	if( !pNodeList ) return;
	
	RS_OVERWRITE_ERROR_NOTIFICATION_T notification;
	long count = pNodeList->Getlength();
	notification.validTargetFilePath = UINT( count );
	for(long i=0; i<count; i++)
	{
		MSXML2::IXMLDOMNodePtr pNode = pNodeList->Getitem(i);
		notification.targetFilePath[i].strRecordingFilePath = CXML::GetText( pNode );
		notification.targetFilePath[i].nFileExist = _ttoi( CXML::GetAttributeValue(pNode, L"is_exist") );
	}
	if( notification.validTargetFilePath>0 ) svcCore->OnOverwritingError( &notification );
}

VOID CALLBACK OnConfigurationChangedCallBack( CONST VOID* pData, size_t nDataSize, VOID* pUserContext )
{
	ServiceCore *svcCore = static_cast<ServiceCore*>( pUserContext );
	if( !svcCore ) 
		return;
	RS_CONFIGURATION_CHANGED_NOTIFICATION_T notification;
	svcCore->OnConfigurationChanged( &notification );
}

VOID CALLBACK OnPlaybackErrorCallBack( CONST VOID* pData, size_t nDataSize, VOID* pUserContext )
{
	ServiceCore *svcCore = static_cast<ServiceCore*>( pUserContext );
	if( !svcCore ) return;
	CHAR *pXML = static_cast<CHAR*>( const_cast<VOID*>(pData) );
	CXML xml;

	RS_PLAYBACK_ERROR_NOTIFICATION_T notification;

	if( !xml.LoadXMLFromString(pXML) ) return;
	MSXML2::IXMLDOMNodePtr pNode = xml.FindNode(_T("//PlaybackID"));
	if( !pNode ) return;
	notification.nPlayBackID = _ttoi( CXML::GetText(pNode) );

	pNode = xml.FindNode(_T("//MAC"));
	if( !pNode ) return;
	notification.strMac = CXML::GetText( pNode );

	pNode = xml.FindNode(_T("//NRSError"));
	if( !pNode ) return;
	notification.strReason = CXML::GetText( pNode );

	pNode = xml.FindNode(_T("//DateTime"));
	if( !pNode ) return;
	CString szTime = pNode->Gettext();
	char *szTime2 = 0;
	ServiceCoordinator::Instance().ConvertWide2MultiByteCharacter( (LPTSTR)(LPCTSTR)szTime, &szTime2 );
	if( szTime2 )
	{
		sscanf( szTime2, "%u-%u-%uT%u:%u:%u", &(notification.year), &(notification.month), &(notification.day), &(notification.hour), &(notification.minute), &(notification.second) );
		free( szTime2 );
	}
	svcCore->OnPlaybackError( &notification );
}

VOID CALLBACK OnDiskErrorCallBack( CONST VOID* pData, size_t nDataSize, VOID* pUserContext )
{
	ServiceCore *svcCore = static_cast<ServiceCore*>( pUserContext );
	if( !svcCore ) return;
	CHAR *pXML = static_cast<CHAR*>( const_cast<VOID*>(pData) );
	CXML xml;

	RS_DISK_ERROR_NOTIFICATION_T notification;

	if( !xml.LoadXMLFromString(pXML) ) 
		return;
	MSXML2::IXMLDOMNodePtr pNode = xml.FindNode(_T("//RecordDiskError"));
	if( !pNode ) 
		return;
	notification.hraFilePath = CXML::GetText( pNode );
	svcCore->OnDiskError( &notification );
}

VOID CALLBACK OnKeyFrameModeCallBack( CONST VOID* pData, size_t nDataSize, VOID* pUserContext )
{
	ServiceCore *svcCore = static_cast<ServiceCore*>( pUserContext );
	if( !svcCore ) return;
	CHAR *pXML = static_cast<CHAR*>( const_cast<VOID*>(pData) );
	CXML xml;

	RS_KEY_FRAME_MODE_NOTIFICATION_T notification;

	if( !xml.LoadXMLFromString(pXML) ) 
		return;
	MSXML2::IXMLDOMNodePtr pNode = xml.FindNode(_T("//RecordingKeyframeMode"));
	if( !pNode ) 
		return;
	notification.strMac = CXML::GetText( pNode );
	svcCore->OnKeyFrameMode( &notification );
}

VOID CALLBACK OnBufferCleanCallBack( CONST VOID* pData, size_t nDataSize, VOID* pUserContext )
{
	ServiceCore *svcCore = static_cast<ServiceCore*>( pUserContext );
	if( !svcCore ) 
		return;
	CHAR *pXML = static_cast<CHAR*>( const_cast<VOID*>(pData) );
	CXML xml;

	RS_BUFFER_CLEAN_NOTIFICATION_T notification;
	if( !xml.LoadXMLFromString(pXML) ) 
		return;
	MSXML2::IXMLDOMNodePtr pNode = xml.FindNode(_T("//RecordingBufferClean"));
	if( !pNode ) 
		return;
	svcCore->OnBufferClean( &notification );
}

ServiceCore::ServiceCore( size_t id, LPCTSTR serverID, VOID *liveSession, HTRecorder *recorder )
: _nId(id)
, _bIsConnecting(FALSE)
, _bIsConnected(FALSE)
, _bBlockMode(FALSE)
, _protocol(NAUTILUS_V2)
, _bAsyncrousConnection(TRUE)
, _liveSession( liveSession )
, _exposedService( recorder )
{
	InitializeCriticalSection( &_cs );
	InitializeCriticalSection( &_csCallback );
	_hEvent = CreateEvent( NULL, TRUE, TRUE, NULL );
	_parser = new XMLParser( this );


	CScopeLock lock( &_csCallback );

	AddNotifyCallback( _T("ConnectionStop"), OnConnectionStopCallBack, this );
	AddNotifyCallback( _T("RecordingStorageFull"), OnRecordingStorageFullCallBack, this );
	AddNotifyCallback( _T("ReservedStorageFull"), OnReservedStorageFullCallBack, this );
	AddNotifyCallback( _T("OverwritingError"), OnOverwritingErrorCallBack, this );
	AddNotifyCallback( _T("ConfigurationChanged"), OnConfigurationChangedCallBack, this );
	AddNotifyCallback( _T("PlaybackError"), OnPlaybackErrorCallBack, this );
	AddNotifyCallback( _T("RecordDiskError"), OnDiskErrorCallBack, this );
	AddNotifyCallback( _T("RecordingKeyframeMode"), OnKeyFrameModeCallBack, this );
	AddNotifyCallback( _T("RecordingBufferClean"), OnBufferCleanCallBack, this );
}

ServiceCore::~ServiceCore( VOID )
{
	{
		CScopeLock lock( &_csCallback );
		RemoveNotifyCallback( _T("ConnectionStop") );
		RemoveNotifyCallback( _T("RecordingStorageFull") );
		RemoveNotifyCallback( _T("ReservedStorageFull") );
		RemoveNotifyCallback( _T("OverwritingError") );
		RemoveNotifyCallback( _T("ConfigurationChanged") );
		RemoveNotifyCallback( _T("PlaybackError") );
		RemoveNotifyCallback( _T("RecordDiskError") );
		RemoveNotifyCallback( _T("RecordingKeyframeMode") );
		RemoveNotifyCallback( _T("RecordingBufferClean") );
	}

	Disconnect();
	CloseHandle( _hEvent );
	DeleteCriticalSection( &_csCallback );
	DeleteCriticalSection( &_cs );
	if( _parser )
	{
		delete _parser;
		_parser = NULL;
	}
}

VOID* ServiceCore::GetHandle( VOID )
{
	return _liveSession;
}

BOOL ServiceCore::Connect( CONST CString& strAddress, CONST UINT nPort, CONST CString& strUser, CONST CString& strPassword, 
						  LiveProtocol protocol, ULONG fileVersion, ULONG dbVersion, BOOL bAsync )
{
	LiveConnect conn;
	BOOL bConnect;
	_strAddress				= strAddress;
	_nPort					= nPort;
	_strUserId				= strUser;
	_strUserPassword		= strPassword;
	_protocol				= protocol;
	_bAsyncrousConnection	= bAsync;
	
	::ZeroMemory( &conn, sizeof(conn) );

	std::string addr = WStringToUTF8(strAddress);
	std::string user = WStringToUTF8(strUser);
	std::string pass = WStringToUTF8(strPassword);

	conn.nIdx			= _nId;
	conn.sAddress		= addr.c_str();
	conn.nPort			= nPort;
	conn.bAsync			= bAsync;
	conn.Protocol		= protocol;
	conn.pReceiver		= this;
	conn.pUserId		= user.c_str();
	conn.pUserPassword	= pass.c_str();
	conn.nVersion		= MAKE_LIVEVERSION( fileVersion, dbVersion );

	_bIsConnecting = bAsync;

	bConnect = Live5_ConnectEx( GetHandle(), &conn );
	if( !bAsync ) _bIsConnected = bConnect;

	return bConnect;
}

BOOL ServiceCore::Reconnect( VOID )
{
	return Connect( _strAddress, _nPort, _strUserId, _strUserPassword, _protocol, _bAsyncrousConnection );
}

VOID ServiceCore::OnNotifyMessage( LiveNotifyMsg* notify )
{
	while( !TryEnterCriticalSection(&_cs) )
	{
		// 연결해제시에만....
		if( notify->nMessage==LIVE_DISCONNECT )
		{
			_bIsConnecting	= FALSE;
			_bIsConnected	= FALSE;
		}
	}

	EnterCriticalSection( &_csCallback );
	_controlCallbackList.clear();
	LeaveCriticalSection( &_csCallback );

	if( notify->nMessage==LIVE_CONNECT )
	{
		_bIsConnecting = FALSE;
		if( notify->nError==NO_ERROR ) _bIsConnected = TRUE;
		else _bIsConnected = FALSE;

		TRACE(_T("NRS 연결: %s:%d, Error: %d\n"), _strAddress, _nPort, notify->nError );
		if( notify->nError==NO_ERROR )
		{
			//Meta 관련 연결
			DWORD dwStreamFlags = RS_RL_FRAME_META;
		}
	}
	else if( notify->nMessage==LIVE_DISCONNECT )
	{
		_bIsConnecting	= FALSE;
		_bIsConnected	= FALSE;
		TRACE(_T("NRS 연결종료 %s:%d, Error: %d\n"), _strAddress, _nPort, notify->nError);
	}
	LeaveCriticalSection( &_cs );
}

VOID ServiceCore::Disconnect( VOID )
{
	//ClearStreamAll();
	Live5_Disconnect( GetHandle(), _nId );
}


VOID ServiceCore::SetAddress( CONST CString& strAddress )
{
	_strAddress = strAddress;
}

VOID ServiceCore::SetPort( CONST UINT nPort )
{
	_nPort = nPort;
}

VOID ServiceCore::SetUser( CONST CString& strUserId, CONST CString& strUserPassword )
{
	_strUserId = strUserId;
	_strUserPassword = strUserPassword;
}

CString ServiceCore::GetUserId( VOID ) CONST
{
	return _strUserId;
}

CString ServiceCore::GetUserPassword( VOID ) CONST
{
	return _strUserPassword;
}


size_t ServiceCore::GetClientID( VOID ) CONST
{
	return _parser->GetClientId();
}

size_t ServiceCore::GetID( VOID ) CONST
{
	return _nId;
}

VOID ServiceCore::SetServerID( LPCTSTR lpServerID )
{
	_strServerID = lpServerID;
}

CString	ServiceCore::GetServerID( VOID ) CONST
{
	return _strServerID;
}

VOID ServiceCore::OnReceive( LPStreamData Data )
{
	if( Data->Type==FRAME_XML )
	{
		_parser->Process( (const char*)Data->pData, Data->nDataSize );
		if( _bBlockMode )
		{
			if( _parser->IsEqualReqId(_strReqID) )
			{
				_sRecvStr = (const char*)Data->pData;
				_strReqID.IsEmpty();
				_bBlockMode = FALSE;
				SetEvent( _hEvent );
			}
		}

		std::wstring req_Id = (LPCTSTR) _parser->GetReqId();
		if( !req_Id.empty() )
		{
			std::map<std::wstring,CONTROL_RECEIVEDATAFUNCTION_T>::iterator pos;
			EnterCriticalSection( &_csCallback );

			pos = _controlCallbackList.find( req_Id );
			if( pos!=_controlCallbackList.end() )
			{
				pos->second.func( Data->pData, Data->nDataSize, pos->second.pUserContext );
				_controlCallbackList.erase( pos );
			}
			LeaveCriticalSection( &_csCallback );
		}
	}
}

int ServiceCore::SendXML( CONST WCHAR* pXML)
{
	std::string utf8 = WStringToUTF8( pXML );
	return SendXML( utf8.c_str() );
}

int ServiceCore::SendXML( CONST CHAR* pXML )
{
	return Live5_SendXML( GetHandle(), _nId, pXML );
}

int ServiceCore::SendXML(const wchar_t* pXML, LPCONTROL_RECEIVEDATAFUNCTION Func, void* pUserContext)
{
	std::string utf8 = WStringToUTF8(pXML);

	return SendXML(utf8.c_str());
}

INT32 ServiceCore::SendXML(const char* pXML, LPCONTROL_RECEIVEDATAFUNCTION Func, void* pUserContext)
{
	INT32 nErr;
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) ) return -1;

	MSXML2::IXMLDOMNodePtr pRoot = xml.GetRootElementPtr();
	CString strId = CXML::GetAttributeValue(pRoot, L"req_id");

	if( strId.IsEmpty() ) return -1;

	CONTROL_RECEIVEDATAFUNCTION_T f;
	f.func			= Func;
	f.pUserContext	= pUserContext;
	_ftime64(&f.t);

	CScopeLock lock( &_csCallback );
	_controlCallbackList.insert( pair<std::wstring, CONTROL_RECEIVEDATAFUNCTION_T>((LPCTSTR)strId, f) );
	nErr = Live5_SendXML( GetHandle(), _nId, pXML );
	if( nErr!=NO_ERROR ) _controlCallbackList.erase( (LPCTSTR)strId );
	return nErr;
}

INT32 ServiceCore::SendRecvXML( std::string& sSend, std::string& sRecv, DWORD dwTimeout )
{
	return SendRecvXML( sSend.c_str(), sRecv, dwTimeout );
}

INT32 ServiceCore::SendRecvXML( CONST WCHAR *pSendXML, std::string& sRecv, DWORD dwTimeout )
{
	std::string utf8 = WStringToUTF8( pSendXML );
	return SendRecvXML( utf8.c_str(), sRecv, dwTimeout );
}

BOOL ServiceCore::IsConnected( BOOL bNoSync )
{
	if( bNoSync ) return _bIsConnected;
	BOOL bConnected;
	{
		CScopeLock lock( &_cs );
		bConnected = _bIsConnected;
	}
	return bConnected; 
}
	
BOOL ServiceCore::IsConnecting( BOOL bNoSync )
{
	if( bNoSync ) return _bIsConnecting;

	BOOL bConnecting;
	{
		CScopeLock lock( &_cs );
		bConnecting = _bIsConnecting;
	}

	return bConnecting;
}

INT32 ServiceCore::SendRecvXML( CONST CHAR* pSendXML, std::string& sRecv, DWORD dwTimeout )
{
	CScopeLock lock( &_cs );
	if( !_bIsConnected ) return -1;

	int nError = NO_ERROR;
	CXML xml;
	if( !xml.LoadXMLFromString(pSendXML) ) return -1;

	MSXML2::IXMLDOMNodePtr pRoot = xml.GetRootElementPtr();
	_strReqID = CXML::GetAttributeValue(pRoot, L"req_id");
	if( _strReqID.IsEmpty() ) return -1;

	_bBlockMode = TRUE;
	ResetEvent( _hEvent );

	nError = Live5_SendXML( GetHandle(), _nId, pSendXML );
	if( nError != NO_ERROR ) return nError;

	DWORD dwTime = 0;
	DWORD dwWait;
	DWORD dwSleep = 500;

	do
	{
		dwWait = WaitForSingleObject( _hEvent, dwSleep );
		if( dwWait==WAIT_TIMEOUT )
		{
			if( dwTimeout!=INFINITE )
			{
			        dwTime += dwSleep;
			        if( dwTime>dwTimeout )
			        {
				        nError = ERROR_TIMEOUT;
				        TRACE(_T("SendRecvXML 타임아웃~~~\n"));
				        break;
			        }
			}

			if( _bIsConnected == FALSE)
			{
				nError = ERROR_CONNECTION_INVALID;
				break;
			}

		}
		else if( dwWait==WAIT_OBJECT_0 )
		{
			sRecv = _sRecvStr;
			nError = NO_ERROR;
			break;
		}	

	} while(1);

	_bBlockMode = FALSE;
	return nError;
}

VOID ServiceCore::SetProtocol( LiveProtocol protocol )
{
	_protocol = protocol;
}

VOID ServiceCore::SetAsyncrousConnection( BOOL bAsync )
{
	_bAsyncrousConnection = bAsync;
}

LiveProtocol ServiceCore::GetProtocol( VOID ) CONST
{
	return _protocol;
}

BOOL ServiceCore::IsAsyncrousConnection( VOID ) CONST
{
	return _bAsyncrousConnection;
}


VOID ServiceCore::AddNotifyCallback( LPCTSTR pNodeName, LPCONTROL_RECEIVEDATAFUNCTION Func, VOID *pUserContext )
{
	_parser->AddNotifyCallback( pNodeName, Func, pUserContext );
}

VOID ServiceCore::RemoveNotifyCallback(LPCTSTR pNodeName)
{
	_parser->RemoveNotifyCallback(pNodeName);
}

///////////////////////  LOGIN  /////////////////////////
BOOL ServiceCore::KeepAliveRequest( VOID )
{
	std::string strSend, strRecv;

	_sprintf_string(strSend, "%s\r\n<KeepAliveReq\"/>", XML_UTF8_HEAD_STR);

	TRACE("(%s #) Send XML = \n%s\n\n", __FUNCTION__, strSend.c_str());

	if(SendRecvXML(strSend.c_str(), strRecv) != NO_ERROR)
		return FALSE;

	if( !CheckErrorCallBack(strRecv.c_str()) )
		return FALSE;
	
	TRACE("(%s #) Recv XML = \n%s\n\n", __FUNCTION__, strRecv.c_str());

	return TRUE;
}

BOOL ServiceCore::GetDeviceList( RS_DEVICE_INFO_SET_T *deviceInfoList )
{
	if( !deviceInfoList ) return FALSE;

	std::string strSend, strRecv;
	CString strXML;
	CString strTemp;

	strXML.Format(_T("%s\r\n<GetAllStreamInfoReq req_id=\"%s\"/>"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );
	strSend = __WTOUTF8( strXML );
	
	TRACE("(%s #) Send XML = \n%s\n\n", __FUNCTION__, strSend.c_str());

	if(SendRecvXML(strSend.c_str(), strRecv) != NO_ERROR)
	{
		TRACE(_T("GetDeviceList error~~~ \n"));
		return FALSE;
	}
	
	TRACE("(%s #) Recv XML = \n%s\n\n", __FUNCTION__, strRecv.c_str());

	CXML xml;
	if( !xml.LoadXMLFromString(strRecv.c_str()) ) return FALSE;
	if( !LoadDevicesCallBack( xml.FindNodeList(_T("//Device")), deviceInfoList ) )
	{
		TRACE(_T("LoadDevicesCallBack error~~~ \n"));
		return FALSE;
	}
	return TRUE;
}

BOOL ServiceCore::CheckDeviceStatus( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_DEVICE_STATUS_SET_T *deviceStatusList )
{
	if( !deviceInfoList || (deviceInfoList->validDeviceCount==0) ) return FALSE;
	if( !deviceStatusList ) return FALSE;

	std::string strSend, strRecv;
	CString strXML, strTemp;

	strXML.Format( _T("%s\r\n<GetStreamStatusReq req_id=\"%s\">\r\n"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );
	for( size_t i=0; i<deviceInfoList->validDeviceCount; i++ )
	{
		strTemp.Format( _T("<StreamID>%s</StreamID>\r\n"), deviceInfoList->deviceInfo[i].GetID() );
		strXML += strTemp;
	}

	strXML += _T("</GetStreamStatusReq>");
	strSend = __WTOUTF8( strXML );
	TRACE("(%s #) Send XML = \n%s\n\n", __FUNCTION__, strSend.c_str());
	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	
	TRACE("(%s #) Recv XML = \n%s\n\n", __FUNCTION__, strRecv.c_str());

	// result 처리
	if( !CheckDeviceStatusCallBack(strRecv.c_str(), deviceStatusList) )
	{
		TRACE("(%s #) CheckDeviceStatusCallBack failed \n", __FUNCTION__);
		return FALSE;
	}

	return TRUE;
}

BOOL ServiceCore::UpdateDevice( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_DEVICE_RESULT_STATUS_SET_T *deviceResultStatusList )
{
	if( !deviceInfoList || (deviceInfoList->validDeviceCount==0) ) return FALSE;
	if( !deviceResultStatusList ) return FALSE;

	std::string strSend, strRecv;
	CString strXML;
	CString strTemp;

	strXML.Format( _T("%s\r\n<UpdateStreamInfoReq req_id=\"%s\">\r\n"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );
	for( UINT index=0; index<deviceInfoList->validDeviceCount; index++ )
	{
		RS_DEVICE_TYPE deviceType = deviceInfoList->deviceInfo[index].GetDeviceType();
		CString strDevType;
		switch( deviceType )
		{
		default:
		case RS_DEVICE_UNKNOWN:
			strDevType = _T("Unknown");
			break;

		case RS_DEVICE_HITRON_CAMERA:
			strDevType = _T("HitronCamera");
			break;

		case RS_DEVICE_ONVIF_CAMERA:
			strDevType = _T("OnvifCamera");
			break;
		}

		strTemp.Format(_T("<Device type=\"%s\">\r\n") \
			_T("<StreamID>%s</StreamID>\r\n") \
			_T("<MAC>%s</MAC>\r\n") \
			_T("<Profile>%s</Profile>\r\n") \
			_T("<ModelType>%s</ModelType>\r\n") \
			_T("<Model>%s</Model>\r\n") \
			_T("<Address>%s</Address>\r\n") \
			_T("<URL>%s</URL>\r\n") \
			_T("<RTSPPort>%d</RTSPPort>\r\n") \
			_T("<HTTPPort>%d</HTTPPort>\r\n") \
			_T("<HTTPSPort>%d</HTTPSPort>\r\n") \
			_T("<SSL>%d</SSL>\r\n") \
			_T("<ConnectType>%d</ConnectType>\r\n") \
			_T("<Login id=\"%s\" pwd=\"%s\"/>\r\n"),

			strDevType,
			deviceInfoList->deviceInfo[index].GetID(),
			deviceInfoList->deviceInfo[index].GetMAC(),
			deviceInfoList->deviceInfo[index].GetProfileName(),
			deviceInfoList->deviceInfo[index].GetModelType(),
			CXML::ConvertSymbol( deviceInfoList->deviceInfo[index].GetModel() ),
			CXML::ConvertSymbol( deviceInfoList->deviceInfo[index].GetAddress() ),
			CXML::ConvertSymbol( deviceInfoList->deviceInfo[index].GetURL() ),

			deviceInfoList->deviceInfo[index].GetRTSPPort(),
			deviceInfoList->deviceInfo[index].GetHTTPPort(),
			deviceInfoList->deviceInfo[index].GetHTTPSPort(),
			deviceInfoList->deviceInfo[index].GetSSL(),
			deviceInfoList->deviceInfo[index].GetConnectionType(),
			CXML::ConvertSymbol( deviceInfoList->deviceInfo[index].GetUser() ),
			CXML::ConvertSymbol( deviceInfoList->deviceInfo[index].GetPassword() ) );

		strXML += strTemp;

		strTemp.Format(_T("<ONVIF>\r\n") \
			_T("<ServiceUri>%s</ServiceUri>\r\n") \
			_T("<PTZ><PTZUri>%s</PTZUri><Token>%s</Token></PTZ>\r\n") \
			_T("</ONVIF>\r\n"),
			CXML::ConvertSymbol(deviceInfoList->deviceInfo[index].GetOnvifServiceUri()),
			CXML::ConvertSymbol(deviceInfoList->deviceInfo[index].GetOnvifPtzUri()),
			CXML::ConvertSymbol(deviceInfoList->deviceInfo[index].GetOnvifPtzToken()));
		strXML += strTemp;
		strXML += _T("</Device>\r\n");

	}

	strXML += _T("</UpdateStreamInfoReq>");

	strSend = __WTOUTF8( strXML );
	
	TRACE("(%s #) Send XML = \n%s\n\n", __FUNCTION__, strSend.c_str());

	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) 
		return FALSE;
	
	TRACE("(%s #) Recv XML = \n%s\n\n", __FUNCTION__, strRecv.c_str());

	// result 처리
	CString strCheck;
	strCheck.Format(_T("//UpdateStreamInfoRes"));
	if( !CommonDeviceCallBack(strRecv.c_str(), deviceResultStatusList, strCheck) )
	{
		TRACE("(%s #) CommonDeviceCallBack failed \n", __FUNCTION__);
		return FALSE;
	}
	return TRUE;
}

BOOL ServiceCore::RemoveDevice( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_DEVICE_RESULT_STATUS_SET_T *deviceResultStatusList )
{
	if( !deviceInfoList || (deviceInfoList->validDeviceCount==0) ) return FALSE;
	if( !deviceResultStatusList ) return FALSE;

	std::string strSend, strRecv;
	CString strXML;
	CString strTemp;

	strXML.Format( _T("%s\r\n<DeleteStreamInfoReq req_id=\"%s\">\r\n"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );
	
	for( size_t index=0; index<deviceInfoList->validDeviceCount; index++)
	{
		strTemp.Format(_T("<StreamID>%s</StreamID>\r\n"), deviceInfoList->deviceInfo[index].GetID() );
		strXML += strTemp;
	}
	
	strXML += strTemp;	
	strXML += _T("</DeleteStreamInfoReq>");

	strSend = __WTOUTF8( strXML );
	
	TRACE("(%s #) Send XML = \n%s\n\n", __FUNCTION__, strSend.c_str());

	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;

	TRACE("(%s #) Recv XML = \n%s\n\n", __FUNCTION__, strRecv.c_str());

	// result 처리
	CString strCheck;
	strCheck.Format(_T("//DeleteStreamInfoRes"));
	if( !CommonDeviceCallBack(strRecv.c_str(), deviceResultStatusList, strCheck) )
	{
		TRACE("(%s #) CommonDeviceCallBack failed \n", __FUNCTION__);
		return FALSE;
	}

	return TRUE;
}

BOOL ServiceCore::RemoveDeviceEx( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_DEVICE_RESULT_STATUS_SET_T *deviceResultStatusList )
{
	if( !deviceInfoList || (deviceInfoList->validDeviceCount==0) ) return FALSE;
	if( !deviceResultStatusList ) return FALSE;

	std::string strSend, strRecv;
	CString strXML;
	CString strTemp;

	strXML.Format( _T("%s\r\n<KillStreamReq req_id=\"%s\">\r\n"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );
	
	for( size_t index=0; index<deviceInfoList->validDeviceCount; index++)
	{
		strTemp.Format(_T("<StreamID>%s</StreamID>\r\n"), deviceInfoList->deviceInfo[index].GetID() );
		strXML += strTemp;
	}
	
	strXML += strTemp;	
	strXML += _T("</KillStreamReq>");

	strSend = __WTOUTF8( strXML );
	
	TRACE("(%s #) Send XML = \n%s\n\n", __FUNCTION__, strSend.c_str());

	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;

	TRACE("(%s #) Recv XML = \n%s\n\n", __FUNCTION__, strRecv.c_str());

	// result 처리
	CString strCheck;
	strCheck.Format(_T("//KillStreamRes"));
	if( !CommonDeviceCallBack(strRecv.c_str(), deviceResultStatusList, strCheck) )
	{
		TRACE("(%s #) CommonDeviceCallBack failed \n", __FUNCTION__);
		return FALSE;
	}

	BOOL bSuccess = FALSE;
	DWORD dwStart = GetTickCount();
	DWORD dwEnd = GetTickCount();
	do
	{
		strSend = "";
		strRecv = "";
		strXML = _T("");
		strTemp = _T("");

		strXML.Format( _T("%s\r\n<IsKilledStreamReq req_id=\"%s\">\r\n"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );

		for( size_t index=0; index<deviceInfoList->validDeviceCount; index++)
		{
			strTemp.Format(_T("<StreamID>%s</StreamID>\r\n"), deviceInfoList->deviceInfo[index].GetID() );
			strXML += strTemp;
		}

		strXML += strTemp;	
		strXML += _T("</IsKilledStreamReq>");

		strSend = __WTOUTF8( strXML );
	
		TRACE("(%s #) Send XML = \n%s\n\n", __FUNCTION__, strSend.c_str());

		if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;

		TRACE("(%s #) Recv XML = \n%s\n\n", __FUNCTION__, strRecv.c_str());

		// result 처리
		strCheck = _T("");
		strCheck.Format(_T("//IsKilledStreamRes"));
		if( !CommonDeviceCallBack(strRecv.c_str(), deviceResultStatusList, strCheck) )
		{
			TRACE("(%s #) CommonDeviceCallBack failed \n", __FUNCTION__);
			return FALSE;
		}


		UINT successedResult = deviceResultStatusList->validDeviceCount;
		for( UINT index=0; index<deviceResultStatusList->validDeviceCount; index++ )
		{
			if( deviceResultStatusList->deviceStatusInfo[index].errorCode==RS_ERROR_NO_ERROR )
			{
				successedResult--;
			}
		}

		if( successedResult==0 ) 
		{
			bSuccess = TRUE;
			break;
		}

		dwEnd = GetTickCount();
		if( dwEnd-dwStart>5000 ) break;
	} while( TRUE );

	if( !bSuccess ) return FALSE;


	strXML = _T("");
	strXML.Format( _T("%s\r\n<DeleteAllRecordingReq req_id=\"%s\">\r\n"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );
	for( UINT index2=0; index2<deviceInfoList->validDeviceCount; index2++ )
	{
		strTemp.Format(_T("<MAC>%s</MAC>\r\n"), deviceInfoList->deviceInfo[index2].GetMAC() );
		strXML += strTemp;
	}

	strXML += _T("</DeleteAllRecordingReq>");
	strSend = __WTOUTF8( strXML );
	TRACE("(%s #) Send XML = \n%s\n", __FUNCTION__, strSend.c_str());
	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	TRACE("(%s #) Recv XML = \n%s\n", __FUNCTION__, strRecv.c_str());
	// result 처리
	strCheck.Format(_T("//DeleteAllRecordingRes"));
	RS_DELETE_RESPONSE_SET_T deleteResponseList;
	if( !DeleteDataResultCallBack(strRecv.c_str(), &deleteResponseList, strCheck) )
	{
		TRACE("(%s #) DeleteDataResultCallBack failed \n", __FUNCTION__);
		return FALSE;
	}

	strXML.Format( _T("%s\r\n<DeleteStreamInfoReq req_id=\"%s\">\r\n"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );	
	for( size_t index=0; index<deviceInfoList->validDeviceCount; index++)
	{
		strTemp.Format(_T("<StreamID>%s</StreamID>\r\n"), deviceInfoList->deviceInfo[index].GetID() );
		strXML += strTemp;
	}
	
	strXML += strTemp;	
	strXML += _T("</DeleteStreamInfoReq>");

	strSend = __WTOUTF8( strXML );
	
	TRACE("(%s #) Send XML = \n%s\n\n", __FUNCTION__, strSend.c_str());

	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;

	TRACE("(%s #) Recv XML = \n%s\n\n", __FUNCTION__, strRecv.c_str());

	// result 처리
	strCheck.Format(_T("//DeleteStreamInfoRes"));
	if( !CommonDeviceCallBack(strRecv.c_str(), deviceResultStatusList, strCheck) )
	{
		TRACE("(%s #) CommonDeviceCallBack failed \n", __FUNCTION__);
		return FALSE;
	}

	return TRUE;
}

BOOL ServiceCore::GetRecordingScheduleList( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_RECORD_SCHEDULE_SET_T *recordShcedList )
{
	if( !deviceInfoList || (deviceInfoList->validDeviceCount==0) ) return FALSE;
	if( !recordShcedList ) return FALSE;

	CString strXML, strTemp;
	std::string strSend, strRecv;
	string SendStr, RecvStr;

	strXML.Format( _T("%s\r\n<GetRecordingScheduleReq req_id=\"%s\">\r\n"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );

	for( size_t index=0; index<deviceInfoList->validDeviceCount; index++ )
	{
		strTemp.Format(_T("<StreamID>%s</StreamID>\r\n"), deviceInfoList->deviceInfo[index].GetID() );
		strXML += strTemp;
	}
	strXML += _T("</GetRecordingScheduleReq>");

	strSend = __WTOUTF8(strXML);
	
	TRACE("(%s #) Send XML = \n%s\n\n", __FUNCTION__, strSend.c_str());

	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	
	TRACE("(%s #) Recv XML = \n%s\n\n", __FUNCTION__, strRecv.c_str());

	if( !LoadScheduleListCallBack(strRecv.c_str(), recordShcedList) )
	{
		TRACE("(%s #) LoadScheduleListCallBack failed \n", __FUNCTION__);
		return FALSE;
	}
	return TRUE;
}

BOOL ServiceCore::UpdateRecordingSchedule( RS_RECORD_SCHEDULE_SET_T *recordSchedList, RS_RESPONSE_INFO_SET_T *responseInfoList )
{
	if( !recordSchedList || (recordSchedList->validScheduleCount==0) ) return FALSE;
	if( !responseInfoList ) return FALSE;

	std::string strSend, strRecv;
	CString strXML, strTemp;
	strXML.Format( _T("%s\r\n<UpdateRecordingScheduleReq req_id=\"%s\">\r\n"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );
	
	for( UINT index=0; index<recordSchedList->validScheduleCount; index++ )
	{
		if( recordSchedList->scheduleInfos[index].nRecordingMode==RS_RECORDING_MODE_T_NORMAL )
		{
			strTemp.Format( _T("<RecordingSchedule stream_id=\"%s\">\r\n") \
							_T("<Pre>%d</Pre>\r\n") \
							_T("<Post>%d</Post>\r\n"),
							recordSchedList->scheduleInfos[index].strStreamID,
							recordSchedList->scheduleInfos[index].nPre,		/* 프리 레코딩 시간(초) */
							recordSchedList->scheduleInfos[index].nPost );		/* 포스트 레코딩 시간(초) */
			strXML += strTemp;
			strXML += _T("<Schedule>\r\n");
			strXML += _T("<Time type=\"normal\">\r\n");
			for( UINT index2=0; index2<recordSchedList->scheduleInfos[index].validNormalSchedCount; index2++ )
			{
				strTemp.Format(_T("<Sch id=\"%s\" adv=\"%d\" audio=\"%d\" type=\"%d\" ") \
					_T("sun=\"%d\" mon=\"%d\" tue=\"%d\" wed=\"%d\" thu=\"%d\" fri=\"%d\" sat=\"%d\"></Sch>\r\n"),
					_T(""), 0, 
					recordSchedList->scheduleInfos[index].normalSchedInfo[index2].nAudio, 
					recordSchedList->scheduleInfos[index].normalSchedInfo[index2].nScheduleType, 
					recordSchedList->scheduleInfos[index].normalSchedInfo[index2].nSun_bitflag, 
					recordSchedList->scheduleInfos[index].normalSchedInfo[index2].nMon_bitflag, 
					recordSchedList->scheduleInfos[index].normalSchedInfo[index2].nTue_bitflag, 
					recordSchedList->scheduleInfos[index].normalSchedInfo[index2].nWed_bitflag, 
					recordSchedList->scheduleInfos[index].normalSchedInfo[index2].nThu_bitflag, 
					recordSchedList->scheduleInfos[index].normalSchedInfo[index2].nFri_bitflag, 
					recordSchedList->scheduleInfos[index].normalSchedInfo[index2].nSat_bitflag );

				strXML += strTemp;
			}
			strXML += _T("</Time>\r\n");
			strXML += _T("</Schedule>\r\n");
			strXML += _T("</RecordingSchedule>\r\n");
		}
		else if( recordSchedList->scheduleInfos[index].nRecordingMode==RS_RECORDING_MODE_T_SPECIAL )
		{
			/*
			strTemp.Format( _T("<Time type=\"special\" enable=\"%d\">\r\n"), recordSchedList->scheduleInfos[index].scheduleInfo.specialSchedInfo.nEnable );
			strXML += strTemp;
				
			strTemp.Format( _T("<Sch month=\"%d\" day=\"%d\">\r\n"), recordSchedList->recordScheduleInfo[index].scheduleInfo[index2].specialSchedInfo.nDay );
			strXML += strTemp;

			for( UINT index3=0; index3<recordSchedList->recordScheduleInfo[index].scheduleInfo[index2].specialSchedInfo.validSpecialTimeCount; index3++ )
			{
				strTemp.Format(_T("<Time t=\"%d\" type=\"%d\" audio=\"%d\">\r\n"),
					recordSchedList->recordScheduleInfo[index].scheduleInfo[index2].specialSchedInfo.timeInfo[index3].nHour, 
					recordSchedList->recordScheduleInfo[index].scheduleInfo[index2].specialSchedInfo.timeInfo[index3].nScheduleType, 
					recordSchedList->recordScheduleInfo[index].scheduleInfo[index2].specialSchedInfo.timeInfo[index3].nAudio);
				strXML += strTemp;
			}
			strXML += _T("</Time>\r\n");
			*/
		}

	}
	strXML += _T("</UpdateRecordingScheduleReq>");
	strSend = __WTOUTF8(strXML);
	

	_tprintf( _T("%s\n"), strXML );
	//TRACE( _T("(%s #) Send XML = \n%s\n"), __FUNCTION__, strXML );

	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	
	printf( "%s\n", strRecv.c_str() );
	//TRACE( _T("(%s #) Recv XML = \n%s\n"), __FUNCTION__, __UTF8TOW(strRecv.c_str()) );

	CString strCheck;
	strCheck.Format(_T("//UpdateRecordingScheduleRes"));
	if( !UpdateScheduleResultCallBack(strRecv.c_str(), responseInfoList, strCheck) )
	{
		//TRACE("(%s #) UpdateScheduleResultCallBack failed \n", __FUNCTION__);
		return FALSE;
	}
	return TRUE;
}

BOOL ServiceCore::SetRecordingOverwrite( RS_RECORD_OVERWRITE_INFO_T *overwriteInfo, RS_RESPONSE_INFO_T *responseInfo )
{
	if( !overwriteInfo ) return FALSE;
	if( !responseInfo ) return FALSE;
	CString strXML;
	std::string strSend, strRecv;
	strXML.Format( _T("%s\r\n<RecordingOverwriteReq req_id=\"%s\" onoff=\"%d\"/>\r\n"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW(), overwriteInfo->onoff );

	strSend = __WTOUTF8(strXML);
	//TRACE("(%s #) Send XML = \n%s\n", __FUNCTION__, strSend.c_str());

	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	
	//TRACE("(%s #) Recv XML = \n%s\n", __FUNCTION__, strRecv.c_str());
	if( !OverwriteResultCallBack(strRecv.c_str(), responseInfo) )
	{
		//TRACE("(%s #) OverwriteResultCallBack failed \n", __FUNCTION__);
		return FALSE;
	}
	return TRUE;
}

BOOL ServiceCore::GetRecordingOverwrite( RS_RECORD_OVERWRITE_INFO_T *overwriteInfo )
{
	if( !overwriteInfo ) return FALSE;
	if( !overwriteInfo ) return FALSE;
	CString strXML;
	std::string strSend, strRecv;
	strXML.Format( _T("%s\r\n<GetRecordingOverwriteReq req_id=\"%s\" />\r\n"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW(), overwriteInfo->onoff );

	strSend = __WTOUTF8(strXML);
	//TRACE("(%s #) Send XML = \n%s\n", __FUNCTION__, strSend.c_str());

	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	
	//TRACE("(%s #) Recv XML = \n%s\n", __FUNCTION__, strRecv.c_str());
	if( !GetOverwriteResultCallBack(strRecv.c_str(), overwriteInfo) )
	{
		//TRACE("(%s #) OverwriteResultCallBack failed \n", __FUNCTION__);
		return FALSE;
	}
	return TRUE;
}

BOOL ServiceCore::SetRecordingRetentionTime( RS_RECORD_RETENTION_INFO_T *retentionInfo, RS_RESPONSE_INFO_T *responseInfo )
{
	if( !retentionInfo ) return FALSE;
	if( !responseInfo ) return FALSE;
	CString strXML, strTemp;

	std::string strSend, strRecv;

	strXML.Format( _T("%s\r\n<UpdateRetentionTimeReq req_id=\"%s\" enable=\'%d\'>\r\n"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW(), retentionInfo->enable );
	
	strTemp.Format(_T("<Year>%d</Year>\r\n") \
		_T("<Month>%d</Month>\r\n") \
		_T("<Week>%d</Week>\r\n") \
		_T("<Day>%d</Day>\r\n"),
		retentionInfo->year, retentionInfo->month, retentionInfo->week, retentionInfo->day );
	strXML += strTemp;
	strXML += _T("</UpdateRetentionTimeReq>");
	strSend = __WTOUTF8(strXML);
	//TRACE("(%s #) Send XML = \n%s\n", __FUNCTION__, strSend.c_str());
	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	
	//TRACE("(%s #) Recv XML = \n%s\n", __FUNCTION v__, strRecv.c_str());
	if( !UpdateRetentionResultCallBack(strRecv.c_str(), responseInfo) )
	{
		//TRACE("(%s #) RetentionResultCallBack failed \n", __FUNCTION__);
		return FALSE;
	}
	return TRUE;
}

BOOL ServiceCore::GetRecordingRetentionTime( RS_RECORD_RETENTION_INFO_T *retentionInfo )
{
	if( !retentionInfo ) return FALSE;
	CString strXML, strTemp;

	std::string strSend, strRecv;

	strXML.Format( _T("%s\r\n<GetRetentionTimeReq req_id=\"%s\"/>\r\n"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW(), retentionInfo->enable );
	strSend = __WTOUTF8(strXML);
	//TRACE("(%s #) Send XML = \n%s\n", __FUNCTION__, strSend.c_str());
	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	
	//TRACE("(%s #) Recv XML = \n%s\n", __FUNCTION v__, strRecv.c_str());
	if( !GetRetentionResultCallBack(strRecv.c_str(), retentionInfo) )
	{
		//TRACE("(%s #) RetentionResultCallBack failed \n", __FUNCTION__);
		return FALSE;
	}
	return TRUE;
}

BOOL ServiceCore::GetDiskInfo( RS_DISK_INFO_SET_T *diskInfoList )
{
	if( !diskInfoList ) return FALSE;
	string SendStr, RecvStr;
	_sprintf_string( SendStr, "%s\r\n<GetDiskInfoReq req_id=\"%s\"/>", XML_UTF8_HEAD_STR, ServiceCoordinator::Instance().GetGUIDA() );

	//TRACE("(%s #) Send XML = \n%s\n", __FUNCTION__, SendStr.c_str());

	if( SendRecvXML(SendStr.c_str(), RecvStr)!=NO_ERROR ) return FALSE;

	//TRACE("(%s #) Recv XML = \n%s\n", __FUNCTION__, RecvStr.c_str());

	if( !LoadDisksCallBack(RecvStr.c_str(), diskInfoList) )
	{
		//TRACE("(%s #) LoadDisksCallBack failed \n", __FUNCTION__);
		return FALSE;
	}
	return TRUE;
}

BOOL ServiceCore::ReserveDiskSpace( RS_DISK_INFO_SET_T *diskInfoList, RS_DISK_RESPONSE_SET_T *diskResponseInfoList )
{
	if( !diskInfoList || (diskInfoList->validDiskCount<1) ) return FALSE;
	if( !diskResponseInfoList ) return FALSE;

	CString strXML, strTemp;
	std::string strSend, strRecv;
	strXML.Format( _T("%s\r\n<ReserveDiskSizeReq req_id=\"%s\">\r\n"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );

//	while(itrDisk != DiskMap.end())
	for( UINT index=0; index<diskInfoList->validDiskCount; index++ )
	{
		strTemp.Format( _T("<Drive volume_serial=\"%s\">\r\n") \
			_T("<Size>%I64d</Size>\r\n") \
			_T("</Drive>\r\n"), 
			diskInfoList->diskInfo[index].strVolumeSerial,
			diskInfoList->diskInfo[index].nCommitReserved );
		strXML += strTemp;
	}
	strXML += _T("</ReserveDiskSizeReq>");
	strSend = __WTOUTF8(strXML);
	
	//TRACE("(%s #) Send XML = \n%s\n", __FUNCTION__, strSend.c_str());

	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	
	//TRACE("(%s #) Recv XML = \n%s\n", __FUNCTION__, strRecv.c_str());

	CString strCheck;
	strCheck.Format(_T("//ReserveDiskSizeRes"));
	if( !ReserveDiskResultCallBack(strRecv.c_str(), diskResponseInfoList, strCheck) )
	{
		TRACE("(%s #) _Proc_ReserveDiskResult failed \n", __FUNCTION__);
		return FALSE;
	}
	return TRUE;
}

BOOL ServiceCore::GetDiskPolicy( RS_DISK_POLICY_SET_T *diskPolicyList )
{
	if( !diskPolicyList ) return FALSE;
	string SendStr, RecvStr;
	_sprintf_string( SendStr, "%s\r\n<GetDiskPolicyReq req_id=\"%s\"/>", XML_UTF8_HEAD_STR, ServiceCoordinator::Instance().GetGUIDA() );
	
	//TRACE("(%s #) Send XML = \n%s\n", __FUNCTION__, SendStr.c_str());
	
	if( SendRecvXML(SendStr.c_str(), RecvStr)!=NO_ERROR ) return FALSE;
	
	//TRACE("(%s #) Recv XML = \n%s\n", __FUNCTION__, RecvStr.c_str());
	
	if( !LoadDiskPolicyCallBack(RecvStr.c_str(), diskPolicyList) )
	{
		//TRACE("(%s #) _Proc_LoadDiskPolicy failed \n", __FUNCTION__);
		return FALSE;
	}
	return TRUE;
}

BOOL ServiceCore::UpdateDiskPolicy( RS_DISK_INFO_SET_T *diskInfo, RS_DEVICE_INFO_SET_T *deviceInfoList, RS_DISK_RESPONSE_SET_T *diskResponseList )
{
	if( !diskInfo || (diskInfo->validDiskCount<1) ) return FALSE;
	if( !deviceInfoList || (deviceInfoList->validDeviceCount<1) ) return FALSE;
	if( !diskResponseList ) return FALSE;

	CString strXML, strTemp;
	std::string strSend, strRecv;
	
	strXML.Format( _T("%s\r\n<UpdateDiskPolicyReq req_id=\"%s\">\r\n"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );

	// 드라이브에 따라 장치 등록 셋팅이 가능하므로 드라이브별 Mac List를 따로 보관해야 함.
	// 현재 밑에 코드는 어떤 한개의 드라이브에만 Mac List를 셋팅한것 임.
	//while(itrDisk != DiskMap.end())
	for( UINT index=0; index<diskInfo->validDiskCount; index++ )
	{
		
		if( (diskInfo->diskInfo[index].strVolumeSerial.GetLength()>0) && (diskInfo->diskInfo[index].nCommitReserved>0) )
		{
			strTemp.Format( _T("<Drive volume_serial=\"%s\">\r\n"), diskInfo->diskInfo[index].strVolumeSerial );
			strXML += strTemp;
			
			for( UINT index2=0; index2<deviceInfoList->validDeviceCount; index2++ )
			{
				strTemp.Format(_T("<MAC>%s</MAC>\r\n"), deviceInfoList->deviceInfo[index2].GetMAC() );
				strXML += strTemp;
			}
			strXML += _T("</Drive>\r\n");
		}
	}
	
	strXML += _T("</UpdateDiskPolicyReq>");

	strSend = __WTOUTF8(strXML);
	
	//TRACE("(%s #) Send XML = \n%s\n", __FUNCTION__, strSend.c_str());

	if(SendRecvXML(strSend.c_str(), strRecv) != NO_ERROR)
		return FALSE;
	
	//TRACE("(%s #) Recv XML = \n%s\n", __FUNCTION__, strRecv.c_str());

	CString strCheck;
	strCheck.Format(_T("//UpdateDiskPolicyRes"));
	if( !DiskPolicyResultCallBack(strRecv.c_str(), diskResponseList, strCheck) )
	{
		//TRACE("(%s #) _Proc_DiskPolicyResult failed \n", __FUNCTION__);
		return FALSE;
	}

	return TRUE;
}

BOOL ServiceCore::IsRecording( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_RECORDING_STATUS_SET_T *recordingStatusList )
{
	if( !deviceInfoList || (deviceInfoList->validDeviceCount<1) ) return FALSE;
	if( !recordingStatusList ) return FALSE;

	std::string strSend, strRecv;
	CString strXML, strTemp;

	strXML.Format( _T("%s\r\n<IsRecordingReq req_id=\"%s\">\r\n"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );
	
	for( UINT index=0; index<deviceInfoList->validDeviceCount; index++ )
	{
		strTemp.Format( _T("<StreamID>%s</StreamID>\r\n"), deviceInfoList->deviceInfo[index].GetID() );
		strXML += strTemp;
	}
	strXML += _T("</IsRecordingReq>");

	strSend = __WTOUTF8( strXML );

	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	if( !IsRecordingCallBack(strRecv.c_str(), recordingStatusList) ) return FALSE;
	return TRUE;
}

BOOL ServiceCore::StartRecordingRequest( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_RESPONSE_INFO_SET_T *responseInfoList )
{
	if( !deviceInfoList || (deviceInfoList->validDeviceCount<1) ) return FALSE;
	if( !responseInfoList ) return FALSE;
	TRACE("(%s #) START \n", __FUNCTION__);

	std::string strSend, strRecv;
	CString strXML, strTemp;

	strXML.Format( _T("%s\r\n<StartRecordingReq req_id=\"%s\">\r\n"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );
	
	for( UINT index=0; index<deviceInfoList->validDeviceCount; index++ )
	{
		strTemp.Format( _T("<StreamID>%s</StreamID>\r\n"), deviceInfoList->deviceInfo[index].GetID() );
		strXML += strTemp;
	}
	strXML += _T("</StartRecordingReq>");

	strSend = __WTOUTF8( strXML );

	TRACE("(%s #) Send XML = \n%s\n", __FUNCTION__, strSend.c_str());

	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	
	TRACE("(%s #) Recv XML = \n%s\n", __FUNCTION__, strRecv.c_str());
	
	if( !RecordingResultCallBack(strRecv.c_str(), responseInfoList) )
	{
		TRACE("(%s #) RecordingResultCallBack failed \n", __FUNCTION__);
		return FALSE;
	}
		TRACE("(%s #) END \n", __FUNCTION__);
	return TRUE;
}

BOOL ServiceCore::StopRecordingRequest( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_RESPONSE_INFO_SET_T *responseInfoList )
{
	if( !deviceInfoList || (deviceInfoList->validDeviceCount<1) ) return FALSE;
	if( !responseInfoList ) return FALSE;
	
	TRACE("(%s #) START \n", __FUNCTION__);

	std::string strSend, strRecv;
	CString strXML, strTemp;

	strXML.Format( _T("%s\r\n<StopRecordingReq req_id=\"%s\">\r\n"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );
	
	for( UINT index=0; index<deviceInfoList->validDeviceCount; index++ )
	{
		strTemp.Format(_T("<StreamID>%s</StreamID>\r\n"), deviceInfoList->deviceInfo[index].GetID() );
		strXML += strTemp;
	}

	strXML += _T("</StopRecordingReq>");

	strSend = __WTOUTF8( strXML );
	
	TRACE("(%s #) Send XML = \n%s\n", __FUNCTION__, strSend.c_str());

	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	
	TRACE("(%s #) Recv XML = \n%s\n", __FUNCTION__, strRecv.c_str());
	
	// result 처리
	if( !RecordingResultCallBack(strRecv.c_str(), responseInfoList) )
	{
		TRACE("(%s #) RecordingResultCallBack failed \n", __FUNCTION__);
		return FALSE;
	}

	TRACE("(%s #) END \n", __FUNCTION__);

	return TRUE;
}

BOOL ServiceCore::StartRecordingAll( VOID )
{
	std::string strSend, strRecv;
	_sprintf_string( strSend, "%s\r\n<StartRecordingAllReq req_id=\"%s\" />", XML_UTF8_HEAD_STR, ServiceCoordinator::Instance().GetGUIDA() );

	TRACE("(%s #) Send XML = \n%s\n", __FUNCTION__, strSend.c_str());
	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	return TRUE;
}

BOOL ServiceCore::StopRecordingAll( VOID )
{
	std::string strSend, strRecv;
	_sprintf_string(strSend, "%s<StopRecordingAllReq req_id=\"%s\" />", XML_UTF8_HEAD_STR, ServiceCoordinator::Instance().GetGUIDA() );
	
	TRACE("(%s #) Send XML = \n%s\n", __FUNCTION__, strSend.c_str());

	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	return TRUE;
}

BOOL ServiceCore::StartManualRecording( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_RESPONSE_INFO_SET_T *resposneInfoList )
{
	if( !deviceInfoList || (deviceInfoList->validDeviceCount<1) ) return FALSE;
	if( !resposneInfoList ) return FALSE;

	TRACE("(%s #) START \n", __FUNCTION__);
	std::string strSend, strRecv;
	CString strXML, strTemp;
	
	strXML.Format( _T("%s<StartManualRecordingReq req_id=\"%s\">"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );
	
	for( UINT index=0; index<deviceInfoList->validDeviceCount; index++)
	{
		strTemp.Format(_T("<StreamID>%s</StreamID>"), deviceInfoList->deviceInfo[index].GetID() );
		strXML += strTemp;
	}

	strXML += _T("</StartManualRecordingReq>");

	strSend = __WTOUTF8( strXML );

	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	
	// result 처리
	if( !RecordingResultCallBack(strRecv.c_str(), resposneInfoList) )
	{
		TRACE("(%s #) RecordingResultCallBack failed \n", __FUNCTION__);
		return FALSE;
	}

	TRACE("(%s #) END \n", __FUNCTION__);

	return TRUE;
}

BOOL ServiceCore::StopManualRecording( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_RESPONSE_INFO_SET_T *responseInfList )
{
	TRACE("(%s #) START \n", __FUNCTION__);

	std::string strSend, strRecv;
	CString strXML, strTemp;

	strXML.Format( _T("%s<StopManualRecordingReq req_id=\"%s\">"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );
	
	for( UINT index=0; index<deviceInfoList->validDeviceCount; index++ )
	{
		strTemp.Format(_T("<StreamID>%s</StreamID>"), deviceInfoList->deviceInfo[index].GetID() );
		strXML += strTemp;
	}

	strXML += _T("</StopManualRecordingReq>");

	strSend = __WTOUTF8( strXML );

	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	
	// result 처리
	if( !RecordingResultCallBack(strRecv.c_str(), responseInfList) )
	{
		TRACE("(%s #) RecordingResultCallBack failed \n", __FUNCTION__);
		return FALSE;
	}

	TRACE("(%s #) END \n", __FUNCTION__);
	return TRUE;
}

BOOL ServiceCore::DeleteRecordingData( RS_DEVICE_INFO_SET_T *deviceInfoList, RS_DELETE_RESPONSE_SET_T *deleteResponseList )
{
	if( !deviceInfoList || (deviceInfoList->validDeviceCount<1) ) return FALSE;
	if( !deleteResponseList ) return FALSE;
	TRACE("(%s #) START \n", __FUNCTION__);

	std::string strSend, strRecv;
	CString strXML, strTemp;
	strXML.Format( _T("%s\r\n<DeleteAllRecordingReq req_id=\"%s\">\r\n"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );
	
	for( UINT index=0; index<deviceInfoList->validDeviceCount; index++ )
	{
		strTemp.Format(_T("<MAC>%s</MAC>\r\n"), deviceInfoList->deviceInfo[index].GetMAC() );
		strXML += strTemp;
	}

	strXML += _T("</DeleteAllRecordingReq>");

	strSend = __WTOUTF8( strXML );
	
	TRACE("(%s #) Send XML = \n%s\n", __FUNCTION__, strSend.c_str());

	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	
	TRACE("(%s #) Recv XML = \n%s\n", __FUNCTION__, strRecv.c_str());

	// result 처리
	CString strCheck;
	strCheck.Format(_T("//DeleteAllRecordingRes"));
	if( !DeleteDataResultCallBack(strRecv.c_str(), deleteResponseList, strCheck) )
	{
		TRACE("(%s #) DeleteDataResultCallBack failed \n", __FUNCTION__);
		return FALSE;
	}
	TRACE("(%s #) END \n", __FUNCTION__);
	return TRUE;
}

///////////////// RELAY STREAM

BOOL ServiceCore::GetRelayInfo( VOID *xmlData, RS_RELAY_INFO_T *rlInfo )
{
	rlInfo->isDisconnected = FALSE;
	CXML xml;
	if(xml.LoadXMLFromString((const char*)xmlData))
	{
		MSXML2::IXMLDOMNodePtr pRoot = xml.GetRootElementPtr();
		_bstr_t baseName = pRoot->GetbaseName();
		if(baseName == _bstr_t(L"StartRelayRes"))
		{
			CString str;
			str.Format(_T("//NRSError[@stream_id=\"%s\"]"), rlInfo->szID );
        
			MSXML2::IXMLDOMNodePtr pNode = xml.FindNode( str );
			if(pNode)
			{
				if(pNode->Gettext() == _bstr_t(_T("NoDeviceStreamInformation")))
					rlInfo->isNoDevice = TRUE;
				else if(pNode->Gettext() == _bstr_t(_T("OK")))
				{
					rlInfo->isNoDevice		= FALSE;
					rlInfo->isDisconnected	= FALSE;
				}
				else
				{
					rlInfo->isDisconnected = TRUE;
				}
			}
			else
			{
				rlInfo->isDisconnected = TRUE;
			}
		}
		else if(baseName == _bstr_t(L"ConnectionStop"))
		{
			// TODO:: 정상연결 종료된것으로 판단하자..
			rlInfo->isDisconnected = TRUE;
		}
	}
	return FALSE;
}

BOOL ServiceCore::StartRelay( RS_DEVICE_INFO_T *relayDevice, RS_RELAY_REQUEST_T *rlRequest )
{
	CString sXml;
	sXml.Format(_T("<?xml version=\"1.0\" encoding=\"utf-8\"?>") \
				_T("<StartRelayReq req_id=\"%s\">") \
				_T("<ClientID>%d</ClientID>") \
				_T("<StreamID>%s</StreamID>") \
				_T("<Type>%d</Type>") \
				_T("</StartRelayReq>"), ServiceCoordinator::Instance().GetGUIDW(), _parser->GetClientId(), relayDevice->GetID(), rlRequest->pbFrameType );

	std::string utf8 = WStringToUTF8(sXml);
	VOID *handle = Live5_ConnectStream( GetHandle(), _nId, utf8.c_str(), static_cast<IStreamReceiver5*>(rlRequest->pReceiver) );
	IRelayStreamReceiver *relayStreamReceiver = static_cast<IRelayStreamReceiver*>( rlRequest->pReceiver );
	relayStreamReceiver->SetHandle( handle );
	if( handle==NULL )
	{
		Live5_DisconnectStream( GetHandle(), _nId, handle );
		return FALSE;
	}
	return TRUE;
}

BOOL ServiceCore::UpdateRelay( RS_DEVICE_INFO_T *relayDevice, RS_RELAY_REQUEST_T *rlRequest )
{
	CString sXml, strTemp;
	sXml.Format( _T("%s<UpdateRelayReq req_id=\"%s\">") \
				_T("<ClientID>%d</ClientID>") \
				_T("<StreamID>%s</StreamID>") \
				_T("<Type>%d</Type>") \
				_T("</UpdateRelayReq>") , 
				XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW(), _parser->GetClientId(), relayDevice->GetID(), rlRequest->pbFrameType );

	sXml += strTemp;
	std::string utf8 = WStringToUTF8(sXml);
	if( SendXML(utf8.c_str())!=NO_ERROR ) return FALSE;
	return TRUE;
}

BOOL ServiceCore::StopRelay( RS_RELAY_INFO_T *rlInfo )
{
	//ControlStop( pbInfo );
	IRelayStreamReceiver *relayStreamReceiver = static_cast<IRelayStreamReceiver*>( rlInfo->pReceiver );
	Live5_DisconnectStream( GetHandle(), _nId, relayStreamReceiver->GetHandle() );
	return TRUE;
}

///////////////// PLAYBACK STREAM
//CALENDAR SEARCH
BOOL ServiceCore::GetYearIndex( RS_PLAYBACK_DEVICE_SET_T *pbDeviceList, INT year/*, CBitArray *monthList*/ )
{
	if( !pbDeviceList || (pbDeviceList->validDeviceCount<1) ) return FALSE;
	std::string strSend, strRecv;

	CString strXML, strTemp;
	strXML.Format( _T("%s<GetYearIndexReq req_id=\"%s\">"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );

	for( UINT index=0; index<pbDeviceList->validDeviceCount; index++)
	{
		strTemp.Format(_T("<StreamID>%s</StreamID>"), pbDeviceList->deviceInfo[index].GetID() );
		strXML += strTemp;
	}

	strTemp.Format(_T("<Year>%d</Year>"), year );
	strXML += strTemp;

	strXML += _T("</GetYearIndexReq>");

	strSend = __WTOUTF8(strXML);
	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	
	GetYearIndexCallBack( strRecv.c_str(), pbDeviceList/*, monthList*/ );

	return TRUE;
}

BOOL ServiceCore::GetMonthIndex( RS_PLAYBACK_DEVICE_SET_T *pbDeviceList, INT year, INT month/*, CBitArray *dayList*/ )
{
	if( !pbDeviceList || (pbDeviceList->validDeviceCount<1) ) return FALSE;
	std::string strSend, strRecv;

	CString strXML, strTemp;

	strXML.Format( _T("%s<GetMonthIndexReq req_id=\"%s\">"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );

	for( UINT index=0; index<pbDeviceList->validDeviceCount; index++)
	{
		strTemp.Format(_T("<StreamID>%s</StreamID>"), pbDeviceList->deviceInfo[index].GetID() );
		strXML += strTemp;
	}	

	strTemp.Format( _T("<Year>%d</Year><Month>%d</Month>"), year, month );
	strXML += strTemp;

	strXML += _T("</GetMonthIndexReq>");

	strSend = __WTOUTF8(strXML);

	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	GetMonthIndexCallBack( strRecv.c_str(), pbDeviceList/*, dayList*/ );
	return TRUE;
}

BOOL ServiceCore::GetDayIndex( RS_PLAYBACK_DEVICE_SET_T *pbDeviceList, INT year, INT month, INT day/*, CBitArray *hourList*/ )
{
	if( !pbDeviceList || (pbDeviceList->validDeviceCount<1) ) return FALSE;
	std::string strSend, strRecv;

	CString strXML, strTemp;

	strXML.Format( _T("%s<GetDayIndexReq req_id=\"%s\">"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );

	for( UINT index=0; index<pbDeviceList->validDeviceCount; index++)
	{
		strTemp.Format( _T("<StreamID>%s</StreamID>"), pbDeviceList->deviceInfo[index].GetID() );
		strXML += strTemp;
	}

	strTemp.Format( _T("<Year>%d</Year><Month>%d</Month><Day>%d</Day>"), year, month, day );
	strXML += strTemp;

	strXML += _T("</GetDayIndexReq>");

	strSend = __WTOUTF8(strXML);

	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	GetDayIndexCallBack( strRecv.c_str(), pbDeviceList/*, hourList*/ );
	return TRUE;
}

BOOL ServiceCore::GetHourIndex( RS_PLAYBACK_DEVICE_SET_T *pbDeviceList, INT year, INT month, INT day, INT hour/*, CBitArray *minuteList, CBitArray *dupMinuteList*/ )
{
	if( !pbDeviceList || (pbDeviceList->validDeviceCount<1) ) return FALSE;
	std::string strSend, strRecv;

	CString strXML, strTemp;
	strXML.Format( _T("%s<GetHourIndexReq req_id=\"%s\">"), XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW() );

	for( UINT index=0; index<pbDeviceList->validDeviceCount; index++)
	{
		strTemp.Format(_T("<StreamID>%s</StreamID>"), pbDeviceList->deviceInfo[index].GetID() );
		strXML += strTemp;
	}

	strTemp.Format( _T("<Year>%d</Year><Month>%d</Month><Day>%d</Day><Hour>%d</Hour>"), year, month, day, hour );
	strXML += strTemp;

	strXML += _T("</GetHourIndexReq>");

	strSend = __WTOUTF8(strXML);

	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;

	GetHourIndexCallBack( strRecv.c_str(), pbDeviceList/*, minuteList, dupMinuteList*/ );
	return TRUE;
}

//PLAYBACK
BOOL ServiceCore::StartPlayback( RS_PLAYBACK_DEVICE_SET_T *pbDeviceList, RS_PLAYBACK_REQUEST_T *pbRequest, RS_PLAYBACK_INFO_T *pbInfo )
{
	CString strXML, strTemp;
	if( !pbDeviceList ) return FALSE;

	strXML.Format( _T("%s<StartPlaybackReq req_id=\"%s\"><ClientID>%d</ClientID>"), 
					XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW(), _parser->GetClientId() );

	for( UINT index=0; index<pbDeviceList->validDeviceCount; index++ )
	{
		strTemp.Format( _T("<StreamID>%s</StreamID>"), pbDeviceList->deviceInfo[index].GetID() );
		strXML += strTemp;
	}

	strTemp.Format( _T("<StartDateTime>%.4d-%.2d-%.2dT%.2d:%.2d:%.2d</StartDateTime>"), 
					pbRequest->year, 
					pbRequest->month, 
					pbRequest->day, 
					pbRequest->hour, 
					pbRequest->minute,
					pbRequest->second );
	strXML += strTemp;

	strTemp.Format(_T("<Type>%d</Type>"), pbRequest->pbFrameType );
	strXML += strTemp;

	// TODO : nOverlapID
/*
	strTemp.Format(_T("<OverlapID>%d</OverlapID>"), 0 );
	strXML += strTemp;
*/
	if( pbRequest->pbDirection!=RS_PLAYBACK_DIRECTION_T_NONE )
	{
		strTemp.Format( _T("<Direction>%s</Direction>"), (pbRequest->pbDirection==RS_PLAYBACK_DIRECTION_T_FORWARD)?_T("Forward"):_T("Backward") );
		strXML += strTemp;
	}
	strXML += _T("</StartPlaybackReq>");
	TRACE( _T("%s\n"), strXML );
	std::string utf8 = WStringToUTF8(strXML);
	VOID *handle = Live5_ConnectStream( GetHandle(), _nId, utf8.c_str(), static_cast<IStreamReceiver5*>(pbRequest->pReceiver) );
	IPlayBackStreamReceiver *pbStreamReceiver = static_cast<IPlayBackStreamReceiver*>( pbRequest->pReceiver );
	pbStreamReceiver->SetHandle( handle );
	if( handle==NULL )
	{
		Live5_DisconnectStream( GetHandle(), _nId, handle );
		return FALSE;
	}
	return TRUE;
}

BOOL ServiceCore::GetPlaybackInfo( VOID *xmlData, RS_PLAYBACK_INFO_T *pbInfo )
{
	CXML xml;
	if(xml.LoadXMLFromString((const char*)xmlData))
	{
/*
<StartPlaybackRes>
  <NRSError>OK</NRSError>
  <PlaybackID>1</PlaybackID> <!-- 플레이백 session연결에 성공한 후 부여되는 ID, 실패하면 0 -->
</StartPlaybackRes>
*/
		MSXML2::IXMLDOMNodePtr pNode = xml.FindNode(_T("//StartPlaybackRes"));
		if( pNode ) 
		{
			RS_ERROR_TYPE errorType = CheckRSErrorCallBack( CXML::GetChildNode(pNode, _T("NRSError")) );
			if( errorType==RS_ERROR_NO_ERROR ) pbInfo->Enable = TRUE;
			else pbInfo->Enable = FALSE;
			pbInfo->playbackID = _ttoi( CXML::GetChildNodeText(pNode, _T("PlaybackID")) );
		}
	}
	return TRUE;
}

BOOL ServiceCore::StopPlayback( RS_PLAYBACK_INFO_T *pbInfo )
{
	//ControlStopEx( pbInfo );
	IPlayBackStreamReceiver *pbStreamReceiver = static_cast<IPlayBackStreamReceiver*>( pbInfo->pReceiver );
	Live5_DisconnectStream( GetHandle(), _nId, pbStreamReceiver->GetHandle() );
	return TRUE;
}

//PLAYBACK CONTROL
BOOL ServiceCore::ControlPlay( RS_PLAYBACK_INFO_T *pbInfo )
{
	std::string strSend;

	TCHAR szSpeed[50] = {0};
	CString szDirection;

	if( pbInfo->pbDirection==RS_PLAYBACK_DIRECTION_T_FORWARD ) szDirection = _T("Forward");
	else szDirection = _T("Backward");

	if( pbInfo->pbSpeed!=RS_PLAYBACK_SPEED_T_MAX ) _itot_s( UINT(pbInfo->pbSpeed), szSpeed, 10 );
	else _tprintf_s( szSpeed, _T("%s"), _T("MAX") );

	CString strXML;
	strXML.Format( _T("%s<ControlPlaybackReq req_id=\"%s\">") \
					_T("<PlaybackID>%d</PlaybackID>") \
					_T("<Direction>%s</Direction>") \
					_T("<Speed>%s</Speed>") \
					_T("<OnlyKeyFrame>%d</OnlyKeyFrame>") \
					_T("</ControlPlaybackReq>"),
					XML_UTF8_HEAD_WSTR,
					ServiceCoordinator::Instance().GetGUIDW(),
					pbInfo->playbackID,
					szDirection,
					szSpeed,
					pbInfo->bOnlyKeyFrame );
	strSend = __WTOUTF8(strXML);
	return (SendXML(strSend.c_str()) == NO_ERROR);
}

BOOL ServiceCore::ControlFowardPlay( RS_PLAYBACK_INFO_T *pbInfo )
{
	BOOL res = FALSE;
	if( !pbInfo ) return FALSE;

	TRACE("(%s #) START  START \n", __FUNCTION__);
	pbInfo->pbDirection = RS_PLAYBACK_DIRECTION_T_FORWARD;
	switch( pbInfo->pbSpeed )
	{
	case RS_PLAYBACK_SPEED_T_1X:
		TRACE(" RS_PLAYBACK_SPEED_T_1X \n", __FUNCTION__);
		res = ControlPlay( pbInfo );
		break;

	case RS_PLAYBACK_SPEED_T_2X:
		TRACE(" RS_PLAYBACK_SPEED_T_2X \n", __FUNCTION__);
		res = ControlPlay( pbInfo );
		break;

	case RS_PLAYBACK_SPEED_T_3X:
		TRACE(" RS_PLAYBACK_SPEED_T_3X \n", __FUNCTION__);
		res = ControlPlay( pbInfo );
		break;

	case RS_PLAYBACK_SPEED_T_4X:
		TRACE(" RS_PLAYBACK_SPEED_T_4X \n", __FUNCTION__);
		res = ControlPlay( pbInfo );
		break;

	case RS_PLAYBACK_SPEED_T_5X:
		TRACE(" RS_PLAYBACK_SPEED_T_5X \n", __FUNCTION__);
		res = ControlPlay( pbInfo );
		break;

	case RS_PLAYBACK_SPEED_T_MAX:
		TRACE(" RS_PLAYBACK_SPEED_T_MAX \n", __FUNCTION__);
		res = ControlPlay( pbInfo );
		break;

	}

	if( res==FALSE )
	{
		TRACE("(%s #) Control PLAY  failed \n", __FUNCTION__);
		return FALSE;
	}
	TRACE("(%s #) END  END \n", __FUNCTION__);
	return TRUE;
}

BOOL ServiceCore::ControlBackwardPlay( RS_PLAYBACK_INFO_T *pbInfo )
{
	BOOL res = FALSE;
	if( !pbInfo ) return FALSE;

	TRACE("(%s #) START  START \n", __FUNCTION__);
	pbInfo->pbDirection = RS_PLAYBACK_DIRECTION_T_BACKWARD;
	switch( pbInfo->pbSpeed )
	{
	case RS_PLAYBACK_SPEED_T_1X:
		TRACE(" RS_PLAYBACK_SPEED_T_1X \n", __FUNCTION__);
		res = ControlPlay( pbInfo );
		break;
		
	case RS_PLAYBACK_SPEED_T_2X:
		TRACE(" RS_PLAYBACK_SPEED_T_2X \n", __FUNCTION__);
		res = ControlPlay( pbInfo );
		break;
		
	case RS_PLAYBACK_SPEED_T_3X:
		TRACE(" RS_PLAYBACK_SPEED_T_3X \n", __FUNCTION__);
		res = ControlPlay( pbInfo );
		break;
		
	case RS_PLAYBACK_SPEED_T_4X:
		TRACE(" RS_PLAYBACK_SPEED_T_4X \n", __FUNCTION__);
		res = ControlPlay( pbInfo );
		break;
		
	case RS_PLAYBACK_SPEED_T_5X:
		TRACE(" RS_PLAYBACK_SPEED_T_5X \n", __FUNCTION__);
		res = ControlPlay( pbInfo );
		break;
		
	case RS_PLAYBACK_SPEED_T_MAX:
		TRACE(" RS_PLAYBACK_SPEED_T_MAX \n", __FUNCTION__);
		res = ControlPlay( pbInfo );
		break;

	}

	if( res==FALSE )
	{
		TRACE("(%s #) Control Reverse-PLAY  failed \n", __FUNCTION__);
		return FALSE;
	}
	TRACE("(%s #) END  END \n", __FUNCTION__);
	return TRUE;
}

BOOL ServiceCore::ControlStop( RS_PLAYBACK_INFO_T *pbInfo )
{
	std::string strSend, strRecv;

	CString strXML;
	strXML.Format( _T("%s<StopPlaybackReq req_id=\"%s\">") \
					_T("<PlaybackID>%d</PlaybackID>") \
					_T("</StopPlaybackReq>"),
					XML_UTF8_HEAD_WSTR,
					ServiceCoordinator::Instance().GetGUIDW(),
					pbInfo->playbackID );

	strSend = __WTOUTF8(strXML);
	TRACE( _T("%s \n"), strXML );
	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	return ControlStopCallBack( strRecv.c_str(), pbInfo );
}

BOOL ServiceCore::ControlPause( RS_PLAYBACK_INFO_T *pbInfo )
{
	std::string strSend, strRecv;

	CString strXML;
	strXML.Format(_T("%s<PausePlaybackReq req_id=\"%s\">") \
					_T("<PlaybackID>%d</PlaybackID>") \
					_T("</PausePlaybackReq>"),
					XML_UTF8_HEAD_WSTR,
					ServiceCoordinator::Instance().GetGUIDW(),
					pbInfo->playbackID );

	strSend = __WTOUTF8( strXML );
	return ( SendXML(strSend.c_str())==NO_ERROR );
}

BOOL ServiceCore::ControlResume( RS_PLAYBACK_INFO_T *pbInfo )
{
	std::string strSend;
	CString strXML;
	strXML.Format( _T("%s<ResumePlaybackReq req_id=\"%s\">") \
					_T("<PlaybackID>%d</PlaybackID>") \
					_T("</ResumePlaybackReq>"),
					XML_UTF8_HEAD_WSTR,
					ServiceCoordinator::Instance().GetGUIDW(),
					pbInfo->playbackID );

	strSend = __WTOUTF8(strXML);
	return ( SendXML(strSend.c_str())==NO_ERROR );
}

BOOL ServiceCore::ControlJump( RS_PLAYBACK_JUMP_REQUEST_T *pbRequest, RS_PLAYBACK_INFO_T *pbInfo )
{
	std::string strSend;
	CString strXML;

	strXML.Format( _T("%s<JumpPlaybackReq req_id=\"%s\">") \
					_T("<PlaybackID>%d</PlaybackID>") \
					_T("<DateTime>%.4d-%.2d-%.2dT%.2d:%.2d:%.2d</DateTime>") \
					_T("</JumpPlaybackReq>"),
					XML_UTF8_HEAD_WSTR,
					ServiceCoordinator::Instance().GetGUIDW(),
					pbInfo->playbackID, 
					pbRequest->year, pbRequest->month, pbRequest->day, pbRequest->hour, pbRequest->minute, pbRequest->second );
	TRACE( _T("%s\n"), strXML );
	strSend = __WTOUTF8(strXML);
	return ( SendXML(strSend.c_str())==NO_ERROR );
}

BOOL ServiceCore::ControlGoToFirst( RS_PLAYBACK_INFO_T *pbInfo, RS_PLAYBACK_GOTO_FIRST_RESPONSE_T *pbResponse )
{
	std::string strSend, strRecv;
	CString strXML;
	strXML.Format( _T("%s<GoToFirstPlaybackReq req_id=\"%s\">") \
					_T("<PlaybackID>%d</PlaybackID>") \
					_T("</GoToFirstPlaybackReq>"),
					XML_UTF8_HEAD_WSTR,
					ServiceCoordinator::Instance().GetGUIDW(),
					pbInfo->playbackID );

	strSend = __WTOUTF8(strXML);
	TRACE( _T("%s \n"), strXML );
	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	return GoToFirstCallBack( strRecv.c_str(), pbResponse );
}

BOOL ServiceCore::ControlGoToLast( RS_PLAYBACK_INFO_T *pbInfo, RS_PLAYBACK_GOTO_LAST_RESPONSE_T *pbResponse )
{
	std::string strSend, strRecv;
	CString strXML;
	strXML.Format( _T("%s<GoToLastPlaybackReq req_id=\"%s\">") \
					_T("<PlaybackID>%d</PlaybackID>") \
					_T("</GoToLastPlaybackReq>"),
					XML_UTF8_HEAD_WSTR,
					ServiceCoordinator::Instance().GetGUIDW(),
					pbInfo->playbackID );

	strSend = __WTOUTF8(strXML);
	TRACE( _T("%s \n"), strXML );
	if( SendRecvXML(strSend.c_str(), strRecv)!=NO_ERROR ) return FALSE;
	return GoToLastCallBack( strRecv.c_str(), pbResponse );
}

BOOL ServiceCore::ControlForwardStep( RS_PLAYBACK_INFO_T *pbInfo )
{
	std::string strSend;
	CString strXML;
	strXML.Format( _T("%s<NextStepPlaybackReq req_id=\"%s\">") \
					_T("<PlaybackID>%d</PlaybackID>") \
					_T("</NextStepPlaybackReq>"),
					XML_UTF8_HEAD_WSTR,
					ServiceCoordinator::Instance().GetGUIDW(),
					pbInfo->playbackID );

	strSend = __WTOUTF8(strXML);
	return ( SendXML(strSend.c_str())==NO_ERROR );
}

BOOL ServiceCore::ControlBackwardStep( RS_PLAYBACK_INFO_T *pbInfo )
{
	std::string strSend;
	CString strXML;
	strXML.Format( _T("%s<PrevStepPlaybackReq req_id=\"%s\">") \
					_T("<PlaybackID>%d</PlaybackID>") \
					_T("</PrevStepPlaybackReq>"),
					XML_UTF8_HEAD_WSTR,
					ServiceCoordinator::Instance().GetGUIDW(),
					pbInfo->playbackID );

	strSend = __WTOUTF8(strXML);
	return ( SendXML(strSend.c_str())==NO_ERROR );
}

///////////////// EXPORT
BOOL ServiceCore::StartExport( RS_DEVICE_INFO_SET_T *devInfoList, RS_EXPORT_REQUEST_T *expRequest, RS_EXPORT_RESPONSE_T *expResponse )
{
	CString strXML, strTemp;
	if( !devInfoList ) return FALSE;

	strXML.Format( _T("%s<StartExportReq req_id=\"%s\"><ClientID>%d</ClientID>"), 
					XML_UTF8_HEAD_WSTR, ServiceCoordinator::Instance().GetGUIDW(), _parser->GetClientId() );

	for( UINT index=0; index<devInfoList->validDeviceCount; index++ )
	{
		strTemp.Format( _T("<StreamID>%s</StreamID>"), devInfoList->deviceInfo[index].GetID() );
		strXML += strTemp;
	}

	strTemp.Format( _T("<StartDateTime>%.4d-%.2d-%.2dT%.2d:%.2d:%.2d</StartDateTime>"), 
					expRequest->sYear, 
					expRequest->sMonth, 
					expRequest->sDay, 
					expRequest->sHour, 
					expRequest->sMinute,
					expRequest->sSecond );
	strXML += strTemp;

	strTemp.Format( _T("<EndDateTime>%.4d-%.2d-%.2dT%.2d:%.2d:%.2d</EndDateTime>"), 
					expRequest->eYear, 
					expRequest->eMonth, 
					expRequest->eDay, 
					expRequest->eHour, 
					expRequest->eMinute,
					expRequest->eSecond );
	strXML += strTemp;

	strTemp.Format( _T("<Type>%d</Type>"), expRequest->type );
	strXML += strTemp;

	//strTemp.Format( _T("<IncludeClipViewer>1</IncludeClipViewer>") );
	//strXML += strTemp;

	// TODO : nOverlapID
/*
	strTemp.Format(_T("<OverlapID>%d</OverlapID>"), 0 );
	strXML += strTemp;
*/
	strXML += _T("</StartExportReq>");
	TRACE( _T("%s\n"), strXML );
	std::string utf8 = WStringToUTF8(strXML);
	VOID *handle = Live5_ConnectStream( GetHandle(), _nId, utf8.c_str(), static_cast<IStreamReceiver5*>(expRequest->pReceiver) );
	IExportStreamReceiver *expStreamReceiver = static_cast<IExportStreamReceiver*>( expRequest->pReceiver );
	expStreamReceiver->SetHandle( handle );
	if( handle==NULL )
	{
		Live5_DisconnectStream( GetHandle(), _nId, handle );
		return FALSE;
	}
	return TRUE;
}

BOOL ServiceCore::StopExport( RS_EXPORT_INFO_T *expInfo )
{
	std::string strSend;
	CString strXML;
	strXML.Format( _T("%s<StopExportReq req_id=\"%s\">") \
					_T("<ExportID>%d</ExportID>") \
					_T("</StopExportReq>"),
					XML_UTF8_HEAD_WSTR,
					ServiceCoordinator::Instance().GetGUIDW(),
					expInfo->exportID );
	TRACE( _T("%s\n"), strXML );
	strSend = __WTOUTF8(strXML);
	return ( SendXML(strSend.c_str())==NO_ERROR );
}

BOOL ServiceCore::PauseExport( RS_EXPORT_INFO_T *expInfo )
{
	std::string strSend;
	CString strXML;
	strXML.Format( _T("%s<PauseExportReq req_id=\"%s\">") \
					_T("<ExportID>%d</ExportID>") \
					_T("</PauseExportReq>"),
					XML_UTF8_HEAD_WSTR,
					ServiceCoordinator::Instance().GetGUIDW(),
					expInfo->exportID );
	TRACE( _T("%s\n"), strXML );
	strSend = __WTOUTF8(strXML);
	return ( SendXML(strSend.c_str())==NO_ERROR );
}

BOOL ServiceCore::ResumeExport( RS_EXPORT_INFO_T *expInfo )
{
	std::string strSend;
	CString strXML;
	strXML.Format( _T("%s<ResumeExportReq req_id=\"%s\">") \
					_T("<ExportID>%d</ExportID>") \
					_T("</ResumeExportReq>"),
					XML_UTF8_HEAD_WSTR,
					ServiceCoordinator::Instance().GetGUIDW(),
					expInfo->exportID );
	TRACE( _T("%s\n"), strXML );
	strSend = __WTOUTF8(strXML);
	return ( SendXML(strSend.c_str())==NO_ERROR );
}

VOID ServiceCore::OnConnectionStop( RS_CONNECTION_STOP_NOTIFICATION_T *notification )
{
	if( _exposedService ) 
		_exposedService->OnConnectionStop( notification );
}

VOID ServiceCore::OnRecordingStorageFull( RS_STORAGE_FULL_NOTIFICATION_T *notification )
{
	if( _exposedService ) 
		_exposedService->OnRecordingStorageFull( notification );
}

VOID ServiceCore::OnReservedStorageFull( RS_STORAGE_FULL_NOTIFICATION_T *notification )
{
	if( _exposedService ) 
		_exposedService->OnReservedStorageFull( notification );
}

VOID ServiceCore::OnOverwritingError( RS_OVERWRITE_ERROR_NOTIFICATION_T *notification )
{
	if( _exposedService ) 
		_exposedService->OnOverwritingError( notification );
}

VOID ServiceCore::OnConfigurationChanged( RS_CONFIGURATION_CHANGED_NOTIFICATION_T *notification )
{
	if( _exposedService ) 
		_exposedService->OnConfigurationChanged( notification );
}

VOID ServiceCore::OnPlaybackError( RS_PLAYBACK_ERROR_NOTIFICATION_T *notification )
{
	if( _exposedService ) 
		_exposedService->OnPlaybackError( notification );
}

VOID ServiceCore::OnDiskError( RS_DISK_ERROR_NOTIFICATION_T *notification )
{
	if( _exposedService ) 
		_exposedService->OnDiskError( notification );
}

VOID ServiceCore::OnKeyFrameMode( RS_KEY_FRAME_MODE_NOTIFICATION_T *notification )
{
	if( _exposedService ) 
		_exposedService->OnKeyFrameMode( notification );
}

VOID ServiceCore::OnBufferClean( RS_BUFFER_CLEAN_NOTIFICATION_T *notification )
{
	if( _exposedService ) 
		_exposedService->OnBufferClean( notification );
}

BOOL ServiceCore::CheckErrorCallBack( CONST CHAR *pRecvStr )
{
	CXML xml;
	if( !xml.LoadXMLFromString(pRecvStr) )
	{
		TRACE("(%s #) LoadXMLFromString failed \n", __FUNCTION__);
		return FALSE;
	}

	MSXML2::IXMLDOMNodeListPtr pErrorList = xml.FindNodeList(_T("//NRSError"));
	if(!pErrorList)
	{
		TRACE("(%s #) !pErrorList failed \n", __FUNCTION__);
		return FALSE;
	}

	long count = pErrorList->Getlength();
	for(long i=0; i<count; i++)
	{
		MSXML2::IXMLDOMNodePtr pNode = pErrorList->Getitem(i);

		CString	strText = pNode->Gettext();
		TRACE( "(%s #) NRSError = %s \n", __FUNCTION__, WStringToUTF8(strText).c_str() );

		if( pNode->Gettext()==bstr_t(L"OK") ) return TRUE;
	}
	TRACE( "(%s #) NOT OK failed \n", __FUNCTION__ );
	return FALSE;
}

RS_ERROR_TYPE ServiceCore::CheckRSErrorCallBack( MSXML2::IXMLDOMNodePtr pRSError )
{
	UINT	channel_count = 0;
	if( pRSError==NULL ) return RS_ERROR_ERROR;
	
	RS_ERROR_TYPE errorCode = RS_ERROR_ERROR;

	if( pRSError->Gettext()==bstr_t(L"OK") )
	{
		errorCode = RS_ERROR_NO_ERROR;
	}
	else if( pRSError->Gettext() == bstr_t(L"Fail") )
	{
		errorCode = RS_ERROR_FAIL;
	}
	else if( pRSError->Gettext() == bstr_t(L"UnknownRequest") )
	{
		errorCode = RS_ERROR_UnKnownRequest;
	}
	else if( pRSError->Gettext() == bstr_t(L"NoDeviceStreamInformation") )
	{
		errorCode = RS_ERROR_NoDeviceStreamInformation;
	}
	else if( pRSError->Gettext() == bstr_t(L"DeviceDisconnected") )
	{
		errorCode = RS_ERROR_DeviceDisconnected;
	}
	else if( pRSError->Gettext() == bstr_t(L"AlreadyRecording") )
	{
		errorCode = RS_ERROR_AlreadyRecording;
	}
	else if( pRSError->Gettext() == bstr_t(L"AlreadyUsing") )
	{
		errorCode = RS_ERROR_AlreadyUsing;
	}
	else if( pRSError->Gettext() == bstr_t(L"NeedSetupLogin") )
	{
		errorCode = RS_ERROR_NeedSetupLogin;
	}
	return errorCode;
}

BOOL ServiceCore::LoadDevicesCallBack( MSXML2::IXMLDOMNodeListPtr pList, RS_DEVICE_INFO_SET_T *deviceInfoList )
{
	UINT channel_count = 0;
	if( !deviceInfoList ) return FALSE;
	if( pList==NULL )
	{
		deviceInfoList->validDeviceCount = 0;
		return TRUE;
	}

	long count = pList->Getlength();
	deviceInfoList->validDeviceCount = UINT( count );
	for(long i=0; i<count; i++)
	{
		MSXML2::IXMLDOMNodePtr pNode = pList->Getitem(i);
		CString device_type	= CXML::GetAttributeValue(pNode, L"type");
		CString	stream_id	= CXML::GetChildNodeText(pNode, _T("StreamID"));
		CString mac			= CXML::GetChildNodeText(pNode, _T("MAC"));
		CString profile		= CXML::GetChildNodeText(pNode, _T("Profile"));
		CString model_type	= CXML::GetChildNodeText(pNode, _T("ModelType"));	// HModel, SModel, ONVIF
		CString model		= CXML::GetChildNodeText(pNode, _T("Model"));		// model name
		CString address		= CXML::GetChildNodeText(pNode, _T("Address"));
		CString url			= CXML::GetChildNodeText(pNode, _T("URL"));			// connection uri
		UINT rtsp_port		= _ttoi( CXML::GetChildNodeText(pNode, _T("RTSPPort")) );
		UINT http_port		= _ttoi( CXML::GetChildNodeText(pNode, _T("HTTPPort")) );
		UINT https_port		= _ttoi( CXML::GetChildNodeText(pNode, _T("HTTPSPort")) );
		BOOL ssl			= _ttoi( CXML::GetChildNodeText(pNode, _T("SSL")) );
		UINT conn_type		= _ttoi( CXML::GetChildNodeText(pNode, _T("ConnectType")) );

		MSXML2::IXMLDOMNodePtr pLoginPtr = CXML::GetChildNode(pNode, _T("Login"));
		if( pLoginPtr )
		{
			CString id	= CXML::GetAttributeValue(pLoginPtr, L"id");
			CString pwd	= CXML::GetAttributeValue(pLoginPtr, L"pwd");

			deviceInfoList->deviceInfo[i].SetUser(id);
			deviceInfoList->deviceInfo[i].SetPassword(pwd);
		}

		RS_DEVICE_TYPE deviceType;
		if( model_type==_T("ONVIF") )
		{
			deviceType = RS_DEVICE_ONVIF_CAMERA;
			MSXML2::IXMLDOMNodePtr pOnvifPtr = CXML::GetChildNode( pNode, _T("ONVIF") );
			if( pOnvifPtr )
			{
				CString service_uri	= CXML::GetChildNodeText( pOnvifPtr, _T("ServiceUri") );
				MSXML2::IXMLDOMNodePtr pOnvifPtzPtr = CXML::GetChildNode( pOnvifPtr, _T("PTZ") );
				if( pOnvifPtzPtr )
				{
					CString ptz_uri	= CXML::GetChildNodeText( pOnvifPtzPtr, _T("PTZUri") );
					CString token	= CXML::GetChildNodeText( pOnvifPtzPtr, _T("Token") );
					deviceInfoList->deviceInfo[i].SetOnvifPtzUri( ptz_uri );
					deviceInfoList->deviceInfo[i].SetOnvifPtzToken(token);
				}
				deviceInfoList->deviceInfo[i].SetOnvifServiceUri(service_uri);
			}
		}
		else
			deviceType = RS_DEVICE_HITRON_CAMERA;

		deviceInfoList->deviceInfo[i].SetID( stream_id );
		deviceInfoList->deviceInfo[i].SetDeviceType( deviceType );
		deviceInfoList->deviceInfo[i].SetMAC( mac );
		deviceInfoList->deviceInfo[i].SetModelType( model_type );
		deviceInfoList->deviceInfo[i].SetModel( model );
		deviceInfoList->deviceInfo[i].SetProfileName( profile );
		deviceInfoList->deviceInfo[i].SetAddress( address );
		deviceInfoList->deviceInfo[i].SetURL( url );
		deviceInfoList->deviceInfo[i].SetRTSPPort( rtsp_port );
		deviceInfoList->deviceInfo[i].SetHTTPPort( http_port );
		deviceInfoList->deviceInfo[i].SetHTTPSPort( https_port );
		deviceInfoList->deviceInfo[i].SetSSL( ssl );
		deviceInfoList->deviceInfo[i].SetConnectionType( conn_type );
		deviceInfoList->deviceInfo[i].SetName( model + mac );

		channel_count++;
		deviceInfoList->deviceInfo[i].SetChannelCount(channel_count);
	}

	return TRUE;
}

BOOL ServiceCore::CheckDeviceStatusCallBack( CONST CHAR *pXML, RS_DEVICE_STATUS_SET_T *deviceStatusList )
{
	if( !deviceStatusList ) return FALSE;
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) ) return FALSE;
	
	MSXML2::IXMLDOMNodeListPtr pList = xml.FindNodeList(_T("//StreamStatus"));
	if( !pList )
	{
		TRACE("(%s #) !pList failed \n", __FUNCTION__);
		return FALSE;
	}

	long count = pList->Getlength();
	deviceStatusList->validDeviceCount = UINT( count );
	for( long index=0; index<count; index++ )
	{
		MSXML2::IXMLDOMNodePtr pNode = pList->Getitem( index );
		deviceStatusList->deviceStatusInfo[index].errorCode		= RS_ERROR_NO_ERROR;
		deviceStatusList->deviceStatusInfo[index].strStreamID	= CXML::GetAttributeValue( pNode, L"stream_id" );
		MSXML2::IXMLDOMNodePtr pRSErrorPtr = CXML::GetChildNode(pNode, _T("NRSError"));
		if( pRSErrorPtr )
		{
			deviceStatusList->deviceStatusInfo[index].errorCode	= CheckRSErrorCallBack( pRSErrorPtr );
			continue;
		}

		deviceStatusList->deviceStatusInfo[index].nIsConnected		= _ttoi( CXML::GetChildNodeText(pNode, _T("IsConnected")) );
		deviceStatusList->deviceStatusInfo[index].nBitrate			= _ttoi( CXML::GetChildNodeText(pNode, _T("Bitrate")) );
		deviceStatusList->deviceStatusInfo[index].nFPS				= _ttoi( CXML::GetChildNodeText(pNode, _T("FPS")) );
		deviceStatusList->deviceStatusInfo[index].strVideoCodec		= CXML::GetChildNodeText(pNode, _T("VideoCodec"));
		deviceStatusList->deviceStatusInfo[index].strAudioCodec		= CXML::GetChildNodeText(pNode, _T("AudioCodec"));
		deviceStatusList->deviceStatusInfo[index].nAudioBitrate		= _ttoi( CXML::GetChildNodeText(pNode, _T("AudioBitrate")) );
		deviceStatusList->deviceStatusInfo[index].nAudioSamplerate	= _ttoi( CXML::GetChildNodeText(pNode, _T("AudioSamplerate")) );
		deviceStatusList->deviceStatusInfo[index].nIsRecordingError	= _ttoi( CXML::GetChildNodeText(pNode, _T("IsRecordingError")) );
		deviceStatusList->deviceStatusInfo[index].nIsVideoRecording	= _ttoi( CXML::GetChildNodeText(pNode, _T("IsVideoRecording")) );
		deviceStatusList->deviceStatusInfo[index].nIsAudioRecording	= _ttoi( CXML::GetChildNodeText(pNode, _T("IsAudioRecording")) );
		deviceStatusList->deviceStatusInfo[index].nResolutionWidth	= _ttoi( CXML::GetChildNodeText(pNode, _T("ResolutionWidth")) );
		deviceStatusList->deviceStatusInfo[index].nResolutionHeight	= _ttoi( CXML::GetChildNodeText(pNode, _T("ResolutionHeight")) );
		
	//	TRACE(" Status Device %2d, streamID=%s, IsConnected=%d, VideoCodec=%s \n", 
	//		ii, WStringToUTF8(deviceStatus.strStreamID).c_str(), deviceStatus.nIsConnected, 
	//		WStringToUTF8(deviceStatus.strVideoCodec).c_str());

	}
	return TRUE;
}

BOOL ServiceCore::CommonDeviceCallBack( CONST CHAR *pXML,  RS_DEVICE_RESULT_STATUS_SET_T *deviceResultStatusList, LPCTSTR strCheck )
{
	if( !deviceResultStatusList ) return FALSE;
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) ) return FALSE;

	MSXML2::IXMLDOMNodeListPtr pCheckList = xml.FindNodeList( strCheck );
	if(!pCheckList)
	{
		TRACE("(%s #) !pCheckList failed, strCheck=%s \n", __FUNCTION__, WStringToUTF8(strCheck).c_str());
		return FALSE;
	}
	
	MSXML2::IXMLDOMNodeListPtr pErrorList = xml.FindNodeList(_T("//NRSError"));
	if(!pErrorList)
	{
		TRACE("(%s #) !pErrorList failed \n", __FUNCTION__);
		return FALSE;
	}

	long count = pErrorList->Getlength();
	deviceResultStatusList->validDeviceCount = UINT( count );
	for( long index=0; index<count; index++ )
	{
		MSXML2::IXMLDOMNodePtr pNode = pErrorList->Getitem( index );
		
		deviceResultStatusList->deviceStatusInfo[index].strStreamID = CXML::GetAttributeValue(pNode, L"stream_id");
		deviceResultStatusList->deviceStatusInfo[index].errorCode	= CheckRSErrorCallBack( pNode );
		
		TRACE( "%2d, streamID=%s, error_code=%d \n", index, WStringToUTF8(deviceResultStatusList->deviceStatusInfo[index].strStreamID).c_str(), deviceResultStatusList->deviceStatusInfo[index].errorCode );
	}
	return TRUE;
}



BOOL ServiceCore::LoadScheduleListCallBack( CONST CHAR *pXML, RS_RECORD_SCHEDULE_SET_T *recordShcedList )
{
	if( !recordShcedList ) return FALSE;
	UINT channel_count = 0;
	
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) ) return FALSE;

	MSXML2::IXMLDOMNodeListPtr pNodeList;
	pNodeList = xml.FindNodeList(_T("//RecordingSchedule"));
	if(pNodeList == NULL) return FALSE;

	MSXML2::IXMLDOMNodePtr pNode;
	long count = pNodeList->Getlength();
	recordShcedList->validScheduleCount = UINT( count );
	for( UINT index=0; index<recordShcedList->validScheduleCount; index++)
	{
		pNode = pNodeList->Getitem( index );
	
		recordShcedList->scheduleInfos[index].strStreamID	= CXML::GetAttributeValue(pNode, L"stream_id");
		recordShcedList->scheduleInfos[index].nPre			= _ttoi( CXML::GetChildNodeText(pNode, _T("Pre")) );
		recordShcedList->scheduleInfos[index].nPost			= _ttoi( CXML::GetChildNodeText(pNode, _T("Post")) );

		TRACE(" strStreamID = %s \n", WStringToUTF8(recordShcedList->scheduleInfos[index].strStreamID).c_str());
		TRACE(" nPre        = %d \n", recordShcedList->scheduleInfos[index].nPre);
		TRACE(" nPost       = %d \n", recordShcedList->scheduleInfos[index].nPost);
		
		//MSXML2::IXMLDOMNodePtr pSchedulePtr = CXML::GetChildNode(pNode, _T("Time"));
		MSXML2::IXMLDOMNodePtr pSchedulePtr = CXML::GetChildNode(pNode, _T("Schedule"));
		if( pSchedulePtr==NULL ) continue;
					
		// Time list 처리
		MSXML2::IXMLDOMNodePtr pTimeNode = CXML::GetChildNode( pSchedulePtr, _T("Time") );
		if( pTimeNode==NULL ) continue;

		CString strRecType = CXML::GetAttributeValue(pTimeNode, L"type");
		TRACE(" strRecType = %s \n", WStringToUTF8(strRecType).c_str() );
		CString		strRecTypeCompared;
		strRecTypeCompared.Format(_T("special"));

		if( strRecType.Compare(strRecTypeCompared)==0 )	recordShcedList->scheduleInfos[index].nRecordingMode = RS_RECORDING_MODE_T_SPECIAL;
		else recordShcedList->scheduleInfos[index].nRecordingMode = RS_RECORDING_MODE_T_NORMAL;

		// Normal schedule info
		if( recordShcedList->scheduleInfos[index].nRecordingMode==RS_RECORDING_MODE_T_NORMAL )
		{
			TRACE("(%s #) NORMAL Schedule !! \n", __FUNCTION__);

			// Normal Sch list 처리
			MSXML2::IXMLDOMNodeListPtr pNorSchList = pTimeNode->GetchildNodes();
			MSXML2::IXMLDOMNodePtr pNorSchNode;
			long norSch_count = pNorSchList->Getlength();
			recordShcedList->scheduleInfos[index].validNormalSchedCount = UINT( norSch_count );
			TRACE("(%s #) Sch cell count = %d \n", __FUNCTION__, norSch_count);
			for( UINT index2=0; index2<recordShcedList->scheduleInfos[index].validNormalSchedCount; index2++ )
			{
				pNorSchNode = pNorSchList->Getitem( index2 );
					
				recordShcedList->scheduleInfos[index].normalSchedInfo[index2].nAudio = _ttoi( CXML::GetAttributeValue(pNorSchNode, L"audio") );
				recordShcedList->scheduleInfos[index].normalSchedInfo[index2].nScheduleType	= (RS_RECORD_SCHEDULE)_ttoi( CXML::GetAttributeValue(pNorSchNode, L"type") );
				recordShcedList->scheduleInfos[index].normalSchedInfo[index2].nSun_bitflag	= _ttoi( CXML::GetAttributeValue(pNorSchNode, L"sun") );
				recordShcedList->scheduleInfos[index].normalSchedInfo[index2].nMon_bitflag	= _ttoi( CXML::GetAttributeValue(pNorSchNode, L"mon") );
				recordShcedList->scheduleInfos[index].normalSchedInfo[index2].nTue_bitflag	= _ttoi( CXML::GetAttributeValue(pNorSchNode, L"tue") );
				recordShcedList->scheduleInfos[index].normalSchedInfo[index2].nWed_bitflag	= _ttoi( CXML::GetAttributeValue(pNorSchNode, L"wed") );
				recordShcedList->scheduleInfos[index].normalSchedInfo[index2].nThu_bitflag	= _ttoi( CXML::GetAttributeValue(pNorSchNode, L"thu") );
				recordShcedList->scheduleInfos[index].normalSchedInfo[index2].nFri_bitflag	= _ttoi( CXML::GetAttributeValue(pNorSchNode, L"fri") );
				recordShcedList->scheduleInfos[index].normalSchedInfo[index2].nSat_bitflag	= _ttoi( CXML::GetAttributeValue(pNorSchNode, L"sat") );
					
				TRACE(" Sch %d \n", index2 );
				TRACE(" nAudio         = %d \n", recordShcedList->scheduleInfos[index].normalSchedInfo[index2].nAudio );
				TRACE(" nScheduleType  = %d \n", recordShcedList->scheduleInfos[index].normalSchedInfo[index2].nScheduleType );
				TRACE(" nSun_bitflag   = %d \n", recordShcedList->scheduleInfos[index].normalSchedInfo[index2].nSun_bitflag );
				TRACE(" nMon_bitflag   = %d \n", recordShcedList->scheduleInfos[index].normalSchedInfo[index2].nMon_bitflag );
				TRACE(" nTue_bitflag   = %d \n", recordShcedList->scheduleInfos[index].normalSchedInfo[index2].nTue_bitflag );
				TRACE(" nWed_bitflag   = %d \n", recordShcedList->scheduleInfos[index].normalSchedInfo[index2].nWed_bitflag );
				TRACE(" nThu_bitflag   = %d \n", recordShcedList->scheduleInfos[index].normalSchedInfo[index2].nThu_bitflag );
				TRACE(" nFri_bitflag   = %d \n", recordShcedList->scheduleInfos[index].normalSchedInfo[index2].nFri_bitflag );
				TRACE(" nSat_bitflag   = %d \n", recordShcedList->scheduleInfos[index].normalSchedInfo[index2].nSat_bitflag );
			} // for end.
		}
		else // Special schedule info
		{
			/*
			TRACE("(%s #) SPECIAL Schedule !! \n", __FUNCTION__);
				
			MSXML2::IXMLDOMNodePtr pSpSchPtr = CXML::GetChildNode(pTimeNode, _T("Sch"));
			if( pSpSchPtr==NULL ) continue;
				
				
			recordShcedList->recordScheduleInfo[index].scheduleInfo[index2].specialSchedInfo.nEnable	
				= _ttoi( CXML::GetAttributeValue(pTimeNode, L"enable") );
			recordShcedList->recordScheduleInfo[index].scheduleInfo[index2].specialSchedInfo.nMonth	
				= _ttoi( CXML::GetAttributeValue(pSpSchPtr, L"month") );
			recordShcedList->recordScheduleInfo[index].scheduleInfo[index2].specialSchedInfo.nDay
				= _ttoi( CXML::GetAttributeValue(pSpSchPtr, L"day") );
				
			TRACE( " nEnable      = %d \n", recordShcedList->recordScheduleInfo[index].scheduleInfo[index2].specialSchedInfo.nEnable );
			TRACE( " nMonth       = %d \n", recordShcedList->recordScheduleInfo[index].scheduleInfo[index2].specialSchedInfo.nMonth );
			TRACE( " nDay         = %d \n", recordShcedList->recordScheduleInfo[index].scheduleInfo[index2].specialSchedInfo.nDay );
				
			// Special Time list 처리
			MSXML2::IXMLDOMNodeListPtr pSpTimeList = pSpSchPtr->GetchildNodes();
			MSXML2::IXMLDOMNodePtr pSpTimeNode;
			long spTime_count = pSpTimeList->Getlength();
			recordShcedList->recordScheduleInfo[index].scheduleInfo[index2].specialSchedInfo.validSpecialTimeCount = UINT( spTime_count );
			TRACE("(%s #) Time cell count = %d \n", __FUNCTION__, spTime_count);
			for( UINT index3=0; index3<recordShcedList->recordScheduleInfo[index].scheduleInfo[index2].specialSchedInfo.validSpecialTimeCount; index3++ )
			{
				pSpTimeNode = pSpTimeList->Getitem( index3 );
						
				recordShcedList->recordScheduleInfo[index].scheduleInfo[index2].specialSchedInfo.timeInfo[index3].nScheduleType
					= (RS_RECORD_SCHEDULE)_ttoi( CXML::GetAttributeValue(pSpTimeNode, L"type") );
				recordShcedList->recordScheduleInfo[index].scheduleInfo[index2].specialSchedInfo.timeInfo[index3].nHour
					= _ttoi( CXML::GetAttributeValue(pSpTimeNode, L"t") );
				recordShcedList->recordScheduleInfo[index].scheduleInfo[index2].specialSchedInfo.timeInfo[index3].nAudio
					= _ttoi( CXML::GetAttributeValue(pSpTimeNode, L"audio") );

				TRACE( " Time %d \n", index3 );
				TRACE( " nScheduleType  = %d \n", recordShcedList->recordScheduleInfo[index].scheduleInfo[index2].specialSchedInfo.timeInfo[index3].nScheduleType );
				TRACE( " nHour          = %d \n", recordShcedList->recordScheduleInfo[index].scheduleInfo[index2].specialSchedInfo.timeInfo[index3].nHour );
				TRACE( " nAudio         = %d \n", recordShcedList->recordScheduleInfo[index].scheduleInfo[index2].specialSchedInfo.timeInfo[index3].nAudio );
			}
			*/
		}
	}
	return TRUE;
}


BOOL ServiceCore::UpdateScheduleResultCallBack( CONST CHAR *pXML, RS_RESPONSE_INFO_SET_T *responseInfoList, LPCTSTR strCheck )
{
	if( !responseInfoList ) return FALSE;
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) ) return FALSE;

	MSXML2::IXMLDOMNodeListPtr pCheckList = xml.FindNodeList(strCheck);
	if( !pCheckList )
	{
		TRACE("(%s #) !pCheckList failed, strCheck=%s \n", __FUNCTION__, WStringToUTF8(strCheck).c_str());
		return FALSE;
	}
	
	MSXML2::IXMLDOMNodeListPtr pErrorList = xml.FindNodeList(_T("//NRSError"));
	if( !pErrorList )
	{
		TRACE("(%s #) !pErrorList failed \n", __FUNCTION__);
		return FALSE;
	}

	long count = pErrorList->Getlength();
	responseInfoList->validDeviceCount = UINT( count );
	for( UINT index=0; index<responseInfoList->validDeviceCount; index++ )
	{
		MSXML2::IXMLDOMNodePtr pNode = pErrorList->Getitem( index );
		responseInfoList->responseInfo[index].strStreamID = CXML::GetAttributeValue(pNode, L"stream_id");
		responseInfoList->responseInfo[index].errorCode	= CheckRSErrorCallBack( pNode );
		
		TRACE( " %2d, streamID=%s, error_code=%d \n", index, WStringToUTF8(responseInfoList->responseInfo[index].strStreamID).c_str(), responseInfoList->responseInfo[index].errorCode );
	}
	return TRUE;
}

BOOL ServiceCore::OverwriteResultCallBack( CONST CHAR *pRecvStr, RS_RESPONSE_INFO_T *responseInfo )
{
	if( !responseInfo ) return FALSE;
	CXML xml;
	if( !xml.LoadXMLFromString(pRecvStr) ) return FALSE;

	MSXML2::IXMLDOMNodeListPtr pErrorList = xml.FindNodeList(_T("//NRSError"));
	if( !pErrorList ) return FALSE;

	long count = pErrorList->Getlength();
	//responseInfo->
	for( long i=0; i<count; i++ )
	{
		MSXML2::IXMLDOMNodePtr pNode = pErrorList->Getitem(i);
		responseInfo->errorCode	= CheckRSErrorCallBack( pNode );
		TRACE("(%s #) NRSError = %d \n", __FUNCTION__, responseInfo->errorCode );
	}
	return TRUE;
}

BOOL ServiceCore::GetOverwriteResultCallBack( CONST CHAR *pRecvStr, RS_RECORD_OVERWRITE_INFO_T *overwriteInfo )
{
	if( !overwriteInfo ) return FALSE;
	CXML xml;
	if( !xml.LoadXMLFromString(pRecvStr) ) return FALSE;

	MSXML2::IXMLDOMNodePtr pNode = xml.FindNode(_T("//OnOff"));
	if( !pNode ) return FALSE;

	CString szOnOff = CXML::GetText( pNode );
	if( szOnOff.GetLength()>0 )
	{
		overwriteInfo->onoff = ::_ttoi( szOnOff );
		return TRUE;
	}
	else return FALSE;
}

BOOL ServiceCore::UpdateRetentionResultCallBack( CONST CHAR *pRecvStr, RS_RESPONSE_INFO_T *responseInfo )
{
	if( !responseInfo ) return FALSE;
	CXML xml;
	if( !xml.LoadXMLFromString(pRecvStr) ) return FALSE;

	MSXML2::IXMLDOMNodeListPtr pErrorList = xml.FindNodeList(_T("//NRSError"));
	if( !pErrorList ) return FALSE;

	long count = pErrorList->Getlength();
	for(long i=0; i<count; i++)
	{
		MSXML2::IXMLDOMNodePtr pNode = pErrorList->Getitem(i);
		responseInfo->errorCode	= CheckRSErrorCallBack( pNode );
		TRACE( "(%s #) NRSError = %d \n", __FUNCTION__, responseInfo->errorCode );
	}
	return TRUE;
}

BOOL ServiceCore::GetRetentionResultCallBack( CONST CHAR *pRecvStr, RS_RECORD_RETENTION_INFO_T *retentionInfo )
{
	CXML xml;
	if( !xml.LoadXMLFromString(pRecvStr) ) return FALSE;
	MSXML2::IXMLDOMNodePtr pRootNode = xml.FindNode(_T("//GetRetentionTimeRes"));
	if( !pRootNode ) return FALSE;
	retentionInfo->enable = _ttoi( CXML::GetAttributeValue( pRootNode, _T("enable") ) );
	MSXML2::IXMLDOMNodePtr pYearNode = CXML::GetChildNode( pRootNode, _T("Year") );
	if( pYearNode ) retentionInfo->year = _ttoi( CXML::GetText( pYearNode ) );
	MSXML2::IXMLDOMNodePtr pMonthNode = CXML::GetChildNode( pRootNode, _T("Month") );
	if( pMonthNode ) retentionInfo->month = _ttoi( CXML::GetText( pMonthNode ) );
	MSXML2::IXMLDOMNodePtr pWeekNode = CXML::GetChildNode( pRootNode, _T("Week") );
	if( pWeekNode ) retentionInfo->week = _ttoi( CXML::GetText( pWeekNode ) );
	MSXML2::IXMLDOMNodePtr pDayNode = CXML::GetChildNode( pRootNode, _T("Day") );
	if( pDayNode ) retentionInfo->day = _ttoi( CXML::GetText( pDayNode ) );
	return TRUE;
}

BOOL ServiceCore::LoadDisksCallBack( CONST CHAR *pXML, RS_DISK_INFO_SET_T *diskInfoList )
{
	if( !diskInfoList ) return FALSE;
	UINT	channel_count = 0;
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) ) return FALSE;

	MSXML2::IXMLDOMNodeListPtr pDriveList;
	pDriveList = xml.FindNodeList(_T("//Drive"));
	if( pDriveList==NULL ) return FALSE;

	//LOCAL_DISK_INFO DiskInfo;

	MSXML2::IXMLDOMNodePtr pDriveNode;
	long count = pDriveList->Getlength();
	diskInfoList->validDiskCount = UINT( count );
	for( UINT index=0; index<diskInfoList->validDiskCount; index++ )
	{
		pDriveNode = pDriveList->Getitem( index );
		
		diskInfoList->diskInfo[index].strRootPath		= CXML::GetChildNodeText(pDriveNode, _T("RootPath"));
		diskInfoList->diskInfo[index].strRootPath.MakeUpper();
		diskInfoList->diskInfo[index].strDrive			= CXML::GetChildNodeText(pDriveNode, _T("Name"));
		diskInfoList->diskInfo[index].strDrive.MakeUpper();
		diskInfoList->diskInfo[index].strVolumeSerial	= CXML::GetChildNodeText(pDriveNode, _T("VolumeSerial"));
		diskInfoList->diskInfo[index].dbTotal			= DOUBLE( _ttoi64( CXML::GetChildNodeText(pDriveNode, _T("Total")) ) / GIGABYTE );
		diskInfoList->diskInfo[index].dbUsed			= DOUBLE( _ttoi64( CXML::GetChildNodeText(pDriveNode, _T("TotalUsage")) ) / GIGABYTE );
		diskInfoList->diskInfo[index].dbTotalFree		= diskInfoList->diskInfo[index].dbTotal - diskInfoList->diskInfo[index].dbUsed;
		diskInfoList->diskInfo[index].nRecordReserved	= _ttoi64( CXML::GetChildNodeText(pDriveNode, _T("Reserved")) );
		diskInfoList->diskInfo[index].nCommitReserved	= diskInfoList->diskInfo[index].nRecordReserved;
		diskInfoList->diskInfo[index].nRecordUsed		= _ttoi64( CXML::GetChildNodeText(pDriveNode, _T("Usage")) );
		diskInfoList->diskInfo[index].nDriveType			= DRIVE_FIXED;	// 네트웍으로 오는건 다 이걸로 가정한다.
	}
	return TRUE;
}

BOOL ServiceCore::ReserveDiskResultCallBack( CONST CHAR *pXML, RS_DISK_RESPONSE_SET_T *diskResponseInfoList, LPCTSTR strCheck )
{
	if( !diskResponseInfoList ) return FALSE;
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) ) return FALSE;

	MSXML2::IXMLDOMNodeListPtr pCheckList = xml.FindNodeList(strCheck);
	if( !pCheckList ) return FALSE;
	
	MSXML2::IXMLDOMNodeListPtr pErrorList = xml.FindNodeList(_T("//NRSError"));
	if(!pErrorList) return FALSE;

	BOOL value = TRUE;
	long count = pErrorList->Getlength();
	diskResponseInfoList->validDiskCount = UINT( count );
	for( UINT index=0; index<diskResponseInfoList->validDiskCount; index++ )
	{
		MSXML2::IXMLDOMNodePtr pNode = pErrorList->Getitem( index );
		
		diskResponseInfoList->responseInfo[index].strVolumeSerial	= CXML::GetAttributeValue(pNode, L"volume_serial");
		diskResponseInfoList->responseInfo[index].errorCode			= CheckRSErrorCallBack( pNode );
		
		TRACE(" %2d, volume_serial=%s, error_code=%d \n", index, WStringToUTF8(diskResponseInfoList->responseInfo[index].strVolumeSerial).c_str(), diskResponseInfoList->responseInfo[index].errorCode );

		if(diskResponseInfoList->responseInfo[index].errorCode!=RS_ERROR_NO_ERROR)
		{
			value &= FALSE;
		}
	}
	return value;
}

BOOL ServiceCore::LoadDiskPolicyCallBack( CONST CHAR * pXML, RS_DISK_POLICY_SET_T *diskPolicyList )
{
	if( !diskPolicyList ) return FALSE;
	UINT	channel_count = 0;
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) )return FALSE;

	MSXML2::IXMLDOMNodeListPtr pDriveList;
	pDriveList = xml.FindNodeList(_T("//Drive"));
	if( pDriveList==NULL ) return FALSE;

	MSXML2::IXMLDOMNodePtr pDriveNode;
	long count = pDriveList->Getlength();
	diskPolicyList->validDiskCount = UINT( count );
	TRACE("(%s #) Drive count = %d \n", __FUNCTION__, count);
	for( UINT index=0; index<diskPolicyList->validDiskCount; index++)
	{
		pDriveNode = pDriveList->Getitem( index );
		
		diskPolicyList->diskPolicyInfo[index].strVolumeSerial	= CXML::GetAttributeValue(pDriveNode, L"volume_serial");
		TRACE(" strVolumeSerial = %s \n", WStringToUTF8(diskPolicyList->diskPolicyInfo[index].strVolumeSerial).c_str());
		// mac list 처리
		MSXML2::IXMLDOMNodeListPtr pMacList = pDriveNode->GetchildNodes();
		MSXML2::IXMLDOMNodePtr pMacNode;
		long mac_count = pMacList->Getlength();

		TRACE("(%s #) mac_count = %d \n", __FUNCTION__, mac_count);
		for( long index2=0; index2<mac_count; index2++ )
		{
			pMacNode = pMacList->Getitem( index2 );
			CString strMac	= pMacNode->Gettext();
			
			TRACE(" strMac = %s \n", WStringToUTF8(strMac).c_str());
			diskPolicyList->diskPolicyInfo[index].strMAC[index2] = (LPCTSTR)strMac;
		}
	}
	return TRUE;
}

BOOL ServiceCore::DiskPolicyResultCallBack( CONST CHAR * pXML, RS_DISK_RESPONSE_SET_T *diskResponseList, LPCTSTR strCheck )
{
	if( !diskResponseList ) return FALSE;
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) ) return FALSE;
	
	MSXML2::IXMLDOMNodeListPtr pCheckList = xml.FindNodeList(strCheck);
	if( !pCheckList ) return FALSE;
	
	MSXML2::IXMLDOMNodeListPtr pErrorList = xml.FindNodeList(_T("//NRSError"));
	if( !pErrorList ) return FALSE;

	long count = pErrorList->Getlength();
	diskResponseList->validDiskCount = UINT( count );
	for( UINT index=0; index<diskResponseList->validDiskCount; index++ )
	{
		MSXML2::IXMLDOMNodePtr pNode = pErrorList->Getitem( index );
		diskResponseList->responseInfo[index].strVolumeSerial = CXML::GetAttributeValue(pNode, L"volume_serial");
		diskResponseList->responseInfo[index].errorCode	= CheckRSErrorCallBack( pNode );
		TRACE( " %2d, strVolumeSerial=%s, error_code=%d \n", index, WStringToUTF8(diskResponseList->responseInfo[index].strVolumeSerial).c_str(), diskResponseList->responseInfo[index].errorCode );
	}
	return TRUE;
}

BOOL ServiceCore::IsRecordingCallBack( CONST CHAR * pXML, RS_RECORDING_STATUS_SET_T *recordingStatusList )
{
	if( !recordingStatusList ) return FALSE;
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) ) return FALSE;
	
	MSXML2::IXMLDOMNodeListPtr pErrorList = xml.FindNodeList(_T("//OnOff"));
	if( !pErrorList ) return FALSE;

	long count = pErrorList->Getlength();
	recordingStatusList->validDeviceCount = UINT( count );
	for( UINT index=0; index<recordingStatusList->validDeviceCount; index++ )
	{
		MSXML2::IXMLDOMNodePtr pNode = pErrorList->Getitem( index );
		recordingStatusList->recordingStatus[index].strStreamID = CXML::GetAttributeValue( pNode, L"stream_id" );
		recordingStatusList->recordingStatus[index].isRecording = _ttoi( CXML::GetText(pNode) );
	}
	return TRUE;
}

BOOL ServiceCore::RecordingResultCallBack( CONST CHAR * pXML, RS_RESPONSE_INFO_SET_T *responseInfoList )
{
	if( !responseInfoList ) return FALSE;
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) ) return FALSE;
	
	MSXML2::IXMLDOMNodeListPtr pErrorList = xml.FindNodeList(_T("//NRSError"));
	if( !pErrorList ) return FALSE;

	long count = pErrorList->Getlength();
	responseInfoList->validDeviceCount = UINT( count );
	for( UINT index=0; index<responseInfoList->validDeviceCount; index++ )
	{
		MSXML2::IXMLDOMNodePtr pNode = pErrorList->Getitem( index );
		
		//responseInfoList->responseInfo[index].strStreamID	= CXML::GetAttributeValue(pNode, L"req_id");
		responseInfoList->responseInfo[index].errorCode		= CheckRSErrorCallBack( pNode );
		
		TRACE(" %2d, error_code=%d \n", index, responseInfoList->responseInfo[index].errorCode );
	}
	return TRUE;
}

BOOL ServiceCore::DeleteDataResultCallBack( CONST CHAR * pXML, RS_DELETE_RESPONSE_SET_T *deleteResponseList, LPCTSTR strCheck )
{
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) ) return FALSE;
	
	MSXML2::IXMLDOMNodeListPtr pCheckList = xml.FindNodeList(strCheck);
	if( !pCheckList ) return FALSE;
	
	MSXML2::IXMLDOMNodeListPtr pErrorList = xml.FindNodeList(_T("//NRSError"));
	if( !pErrorList ) return FALSE;

	long count = pErrorList->Getlength();
	deleteResponseList->validDeviceCount = UINT( count );
	for( UINT index=0; index<deleteResponseList->validDeviceCount; index++ )
	{
		MSXML2::IXMLDOMNodePtr pNode = pErrorList->Getitem( index );
		
		deleteResponseList->responseInfo[index].strMAC		= CXML::GetAttributeValue(pNode, L"mac");
		deleteResponseList->responseInfo[index].errorCode	= CheckRSErrorCallBack( pNode );
		
		TRACE(" %2d, Mac=%s, error_code=%d \n", index, WStringToUTF8(deleteResponseList->responseInfo[index].strMAC).c_str(), deleteResponseList->responseInfo[index].errorCode );
	}
	return TRUE;
}

BOOL ServiceCore::GetYearIndexCallBack( CONST CHAR * pXML, RS_PLAYBACK_DEVICE_SET_T *pbDeviceList/*, CBitArray *monthList*/ )
{
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) ) return FALSE;

	MSXML2::IXMLDOMNodeListPtr pList = xml.FindNodeList(_T("//YearIndex"));
	if( !pList ) return FALSE;

	long count = pList->Getlength();
	MSXML2::IXMLDOMNodePtr pNode;
	CString sDeviceID;
	string sIndex;
	for( long i=0; i<count; i++ )
	{
		pNode = pList->Getitem(i);
		sDeviceID = CXML::GetAttributeValue(pNode, L"stream_id");

		sIndex = WStringToAString(pNode->Gettext());
		SetMonthIndex( sDeviceID, sIndex.c_str(), pbDeviceList/*, monthList*/ );
	}
	return TRUE;
}

VOID ServiceCore::SetMonthIndex( LPCTSTR deviceID, CONST CHAR *pIndexBase64, RS_PLAYBACK_DEVICE_SET_T *pbDeviceList/*, CBitArray *monthList*/ )
{
	UINT index = 0;
	for( index=0; index<pbDeviceList->validDeviceCount; index++ )
	{
		if( pbDeviceList->deviceInfo[index].GetID()==deviceID ) break;
	}

	if( index<pbDeviceList->validDeviceCount )
	{
		BYTE iMonth[4] = {0};
		INT nLen = SIZEOF_ARRAY(iMonth);
		if( Base64Decode(pIndexBase64, strlen(pIndexBase64), iMonth, &nLen) )
		{
			pbDeviceList->deviceInfo[index].SetRecMonth( iMonth, nLen );
			//monthList->ExclusiveOR(iMonth, nLen);
		}
	}
}

BOOL ServiceCore::GetMonthIndexCallBack( CONST CHAR * pXML, RS_PLAYBACK_DEVICE_SET_T *pbDeviceList/*, CBitArray *dayList*/ )
{
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) ) return FALSE;

	MSXML2::IXMLDOMNodeListPtr pList = xml.FindNodeList(_T("//MonthIndex"));
	if( !pList ) return FALSE;

	//dayList->Reset();
	long count = pList->Getlength();
	MSXML2::IXMLDOMNodePtr pNode;
	CString sDeviceID;
	string sIndex;
	for(long i=0; i<count; i++)
	{
		pNode = pList->Getitem(i);
		sDeviceID = CXML::GetAttributeValue(pNode, L"stream_id");
		sIndex = WStringToAString(pNode->Gettext());
		SetDayIndex( sDeviceID, sIndex.c_str(), pbDeviceList/*, dayList*/ );
	}
	return TRUE;
}

VOID ServiceCore::SetDayIndex( LPCTSTR deviceID, CONST CHAR *pIndexBase64, RS_PLAYBACK_DEVICE_SET_T *pbDeviceList/*, CBitArray *dayList*/ )
{
	UINT index = 0;
	for( index=0; index<pbDeviceList->validDeviceCount; index++ )
	{
		if( pbDeviceList->deviceInfo[index].GetID()==deviceID ) break;
	}

	if( index<pbDeviceList->validDeviceCount )
	{
		BYTE iDay[4] = {0};
		INT nLen = SIZEOF_ARRAY(iDay);
		if( Base64Decode(pIndexBase64, strlen(pIndexBase64), iDay, &nLen) )
		{
			pbDeviceList->deviceInfo[index].SetRecDay( iDay, nLen );
			//dayList->ExclusiveOR( iDay, nLen );
		}
	}
}

BOOL ServiceCore::GetDayIndexCallBack( CONST CHAR * pXML, RS_PLAYBACK_DEVICE_SET_T *pbDeviceList/*, CBitArray *hourList*/  )
{
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) ) return FALSE;

	MSXML2::IXMLDOMNodeListPtr pList = xml.FindNodeList(_T("//DayIndex"));
	if( !pList ) return FALSE;

	//hourList->Reset();
	long count = pList->Getlength();
	MSXML2::IXMLDOMNodePtr pNode;
	CString sDeviceID;
	string sIndex;
	for(long i=0; i<count; i++)
	{
		pNode = pList->Getitem(i);
		sDeviceID = CXML::GetAttributeValue(pNode, L"stream_id");
		sIndex = WStringToAString(pNode->Gettext());
		SetHourIndex( sDeviceID, sIndex.c_str(), pbDeviceList/*, hourList*/ );
	}
	return TRUE;
}

VOID ServiceCore::SetHourIndex( LPCTSTR deviceID, CONST CHAR *pIndexBase64, RS_PLAYBACK_DEVICE_SET_T *pbDeviceList/*, CBitArray* hourList*/ )
{
	UINT index = 0;
	for( index=0; index<pbDeviceList->validDeviceCount; index++ )
	{
		if( pbDeviceList->deviceInfo[index].GetID()==deviceID ) break;
	}

	if( index<pbDeviceList->validDeviceCount )
	{
		BYTE iHour[8] = {0};
		INT nLen = SIZEOF_ARRAY(iHour);
		if( Base64Decode(pIndexBase64, strlen(pIndexBase64), iHour, &nLen) )
		{
			pbDeviceList->deviceInfo[index].SetRecHour( iHour, nLen );
			//hourList->ExclusiveOR(iHour, nLen);
		}
	}
}

BOOL ServiceCore::GetHourIndexCallBack( CONST CHAR * pXML, RS_PLAYBACK_DEVICE_SET_T *pbDeviceList/*, CBitArray *minuteList, CBitArray *dupMinuteList*/ )
{
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) ) return FALSE;
	MSXML2::IXMLDOMNodeListPtr pList = xml.FindNodeList(_T("//HourIndex"));
	if( !pList ) return FALSE;

	//minuteList->Reset();
	//dupMinuteList->Reset();
	long count = pList->Getlength();
	MSXML2::IXMLDOMNodePtr pNode;
	CString sDeviceID;
	string sIndex, sDupIndex;
	for(long i=0; i<count; i++)
	{
		pNode = pList->Getitem(i);
		sDeviceID = CXML::GetAttributeValue(pNode, L"stream_id");

		sIndex		= WStringToAString( CXML::GetChildNodeText(pNode, _T("IsRecorded")) );
		//sDupIndex	= WStringToAString( CXML::GetChildNodeText(pNode, _T("IsOverlapped")) );

		SetMinIndex( sDeviceID, sIndex.c_str(), /*sDupIndex.c_str(), */pbDeviceList/*, minuteList, dupMinuteList*/ );
	}
	return TRUE;
}

VOID ServiceCore::SetMinIndex( LPCTSTR deviceID, CONST CHAR *pIndexBase64, /*CONST CHAR *pDupIndexBase64, */RS_PLAYBACK_DEVICE_SET_T *pbDeviceList/*, CBitArray *minuteList, CBitArray *dupMinuteList*/ )
{
	UINT index = 0;
	for( index=0; index<pbDeviceList->validDeviceCount; index++ )
	{
		if( pbDeviceList->deviceInfo[index].GetID()==deviceID ) break;
	}

	if( index<pbDeviceList->validDeviceCount )
	{
		BYTE iMin[8] = {0};
		//BYTE iDup[8] = {0};
		
		INT nLen1 = SIZEOF_ARRAY(iMin);
		if( Base64Decode(pIndexBase64, strlen(pIndexBase64), iMin, &nLen1) )
		{
			pbDeviceList->deviceInfo[index].SetRecMin( iMin, nLen1 );
			//minuteList->ExclusiveOR( iMin, nLen1 );
		}

		/*
		INT nLen2 = SIZEOF_ARRAY(iDup);
		if( Base64Decode(pDupIndexBase64, strlen(pDupIndexBase64), iDup, &nLen2) )
		{
			pbDeviceList->deviceInfo[index].SetRecDupMin( iDup, nLen2 );
			//dupMinuteList->ExclusiveOR( iDup, nLen2 );
		}
		*/
	}
}

BOOL ServiceCore::ControlStopCallBack( CONST CHAR *pRecvStr, RS_PLAYBACK_INFO_T *pbInfo )
{
	CXML xml;
	if( !xml.LoadXMLFromString(pRecvStr) ) return FALSE;

	MSXML2::IXMLDOMNodePtr pNode = xml.FindNode(_T("//NRSError"));
	if( !pNode ) return FALSE;

	RS_ERROR_TYPE errorCode = CheckRSErrorCallBack( pNode );
	if( errorCode==RS_ERROR_NO_ERROR ) return TRUE;
	else return FALSE;
}

BOOL ServiceCore::GoToFirstCallBack( CONST CHAR * pXML, RS_PLAYBACK_GOTO_FIRST_RESPONSE_T *pbResponse )
{
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) ) return FALSE;
	MSXML2::IXMLDOMNodeListPtr pList = xml.FindNodeList(_T("//DateTime"));
	if( !pList ) return FALSE;

	long count = pList->Getlength();
	CString szTime;
	char *szTime2 = 0;
	CString szYear, szMonth, szDay, szHour, szMinute, szSecond;
	for( long i=0; i<count; i++ )
	{
		MSXML2::IXMLDOMNodePtr pNode = pList->Getitem(i);
		szTime = CXML::GetText( pNode );
		ServiceCoordinator::Instance().ConvertWide2MultiByteCharacter( (LPTSTR)(LPCTSTR)szTime, &szTime2 );
		if( szTime2 )
		{
			sscanf( szTime2, "%u-%u-%uT%u:%u:%u", &(pbResponse->year), &(pbResponse->month), &(pbResponse->day), &(pbResponse->hour), &(pbResponse->minute), &(pbResponse->second) );
			free( szTime2 );
		}
	}
	return TRUE;
}

BOOL ServiceCore::GoToLastCallBack( CONST CHAR * pXML, RS_PLAYBACK_GOTO_LAST_RESPONSE_T *pbResponse )
{
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) ) return FALSE;
	MSXML2::IXMLDOMNodeListPtr pList = xml.FindNodeList(_T("//DateTime"));
	if( !pList ) return FALSE;

	long count = pList->Getlength();
	CString szTime;
	char *szTime2 = 0;
	CString szYear, szMonth, szDay, szHour, szMinute, szSecond;
	for( long i=0; i<count; i++ )
	{
		MSXML2::IXMLDOMNodePtr pNode = pList->Getitem(i);
		szTime = CXML::GetText( pNode );
		ServiceCoordinator::Instance().ConvertWide2MultiByteCharacter( (LPTSTR)(LPCTSTR)szTime, &szTime2 );
		if( szTime2 )
		{
			sscanf( szTime2, "%u-%u-%uT%u:%u:%u", &(pbResponse->year), &(pbResponse->month), &(pbResponse->day), &(pbResponse->hour), &(pbResponse->minute), &(pbResponse->second) );
			free( szTime2 );
		}

	}
	return TRUE;
}
