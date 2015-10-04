#include "Common.h"
#include <XML.h>
#include "XMLParser.h"
#include "ScopeLock.h"
#include "ServiceCoordinator.h"
#include "common/ThreadPool.h"
#include "common/ThreadUtil.h"

XMLParser::XMLParser( ServiceCore *service )
	: _clientId(-1),
	  _service(service)
{
	InitializeCriticalSection( &_lock );
}

XMLParser::~XMLParser( VOID )
{
	DeleteCriticalSection( &_lock );
}

INT32 XMLParser::Process( CONST CHAR *pXML, size_t nSize )
{
	TRACE( pXML );
	TRACE( "\n" );
	CXML xml;
	if( !xml.LoadXMLFromString(pXML) )
	{
		_reqId = _T("");
		return -1;
	}

	INT32 nError = NO_ERROR;
	MSXML2::IXMLDOMNodePtr pRoot = xml.GetRootElementPtr();

	CString BaseName(pRoot->GetbaseName().GetBSTR());
	if( BaseName==_T("GetClientIDRes") )
	{
		nError = _ProcessXML_GetClientIDRes(pRoot);
	}
/*
	else if( (BaseName==_T("ConnectionStop")) ||
			 (BaseName==_T("RecordingStorageFull")) ||	
			 (BaseName==_T("ReservedStorageFull")) ||	
			 (BaseName==_T("OverwritingError")) ||	
			 (BaseName==_T("ConfigurationChanged")) ||	
			 (BaseName==_T("PlaybackError")) )
*/
	else
	{
		map<wstring, LPCONTROL_RECEIVEDATAFUNCTION_T>::iterator pos;
		EnterCriticalSection( &_lock );
		pos = _callbackList.find( (LPCTSTR)BaseName );
		if( pos!=_callbackList.end() ) 
		{
#if 0
			pos->second->func( pXML, nSize, pos->second->pUserContext );
#else
			LPQUEUE_WORKER_ELEMENT_T qElement = static_cast<LPQUEUE_WORKER_ELEMENT_T>( malloc(sizeof(QUEUE_WORKER_ELEMENT_T)) );
			qElement->f = pos->second;
			qElement->nSize = nSize;
			qElement->pXML = _strdup( pXML );
			_queue.push( qElement );
			ServiceCoordinator::Instance().GetThreadPool()->AddWorkerFunction( OnNotification, this, 0, 0 );
#endif
		}
		else
		{
			_reqId = CXML::GetAttributeValue(pRoot, L"req_id");
		}
		LeaveCriticalSection( &_lock );
	}


	return nError;
}

VOID CALLBACK XMLParser::OnNotification( VOID *arg, size_t key )
{
	XMLParser *self = static_cast<XMLParser*>( arg );
	self->ProcessNotification( key );
}

VOID XMLParser::ProcessNotification( size_t key )
{
	EnterCriticalSection( &_lock );

	if(_queue.empty())
	{
		LeaveCriticalSection( &_lock );
		return;
	}

	LPQUEUE_WORKER_ELEMENT_T qElement = _queue.front();             // 큐에서 하나씩 꺼낸다
	_queue.pop();

	qElement->f->func( qElement->pXML, qElement->nSize, qElement->f->pUserContext );


	//delete qElement->f;
	free( qElement->pXML );
	free( qElement );


	LeaveCriticalSection( &_lock );
}


INT32 XMLParser::_ProcessXML_GetClientIDRes( MSXML2::IXMLDOMNodePtr pNode )
{
	_clientId = _ttoi( CXML::GetChildNodeText(pNode, _T("ClientID")) );
	TRACE( _T("_clientID : %d\n"), _clientId );
	return NO_ERROR;
}

INT32 XMLParser::_ProcessXML_GetAllStreamInfoRes( MSXML2::IXMLDOMNodeListPtr pList )
{
	if(pList == NULL) return -1;

	ClearStreamListAll();

	RS_STREAM_INFO_T *pStream;
	MSXML2::IXMLDOMNodePtr pNode;
	long count = pList->Getlength();
	for(long i=0; i<count; i++)
	{
		pNode = pList->Getitem(i);
		pStream = new RS_STREAM_INFO_T;
		pStream->nId			= i;
		pStream->strMAC			= CXML::GetChildNodeText(pNode, _T("MAC"));
		pStream->strProfile		= CXML::GetChildNodeText(pNode, _T("Profile"));
		pStream->strAddress		= CXML::GetChildNodeText(pNode, _T("Address"));
		pStream->strModel		= CXML::GetChildNodeText(pNode, _T("Model"));
		pStream->strModelType	= CXML::GetChildNodeText(pNode, _T("ModelType"));
		pStream->strUrl			= CXML::GetChildNodeText(pNode, _T("URL"));

		_streamMultiMap.insert( pair<wstring, RS_STREAM_INFO_T*>( (LPCTSTR)pStream->strMAC, pStream ) );
	}	

	return NO_ERROR;
}

VOID XMLParser::ClearStreamListAll( void )
{
	std::multimap<wstring, RS_STREAM_INFO_T*>::iterator pos = _streamMultiMap.begin();
	while( pos!=_streamMultiMap.end() )
	{
		delete pos->second;
		pos++;
	}
	_streamMultiMap.clear();
}

INT32 XMLParser::_ProcessXML_LoadMSConfigRes( MSXML2::IXMLDOMNodeListPtr pList )
{
	return NO_ERROR;
}

BOOL XMLParser::IsEqualReqId( CString& strReqId )
{
	if( _reqId.IsEmpty() ) return FALSE;
	if( _reqId==strReqId )
	{
		_reqId.Empty();
		return TRUE;
	}
	return FALSE;
}

VOID XMLParser::AddNotifyCallback( LPCTSTR pNodeName, LPCONTROL_RECEIVEDATAFUNCTION Func, void* pUserContext )
{
	if( pNodeName==NULL ) return;
	LPCONTROL_RECEIVEDATAFUNCTION_T f = new CONTROL_RECEIVEDATAFUNCTION_T;
	f->func			= Func;
	f->pUserContext	= pUserContext;
	_ftime64_s(&f->t);

	wstring nodeName(pNodeName);
	CScopeLock lock( &_lock );
	_callbackList[nodeName] = f;
}

VOID XMLParser::RemoveNotifyCallback( LPCTSTR pNodeName )
{
	if( pNodeName==NULL )
	{
		// Remove all
		CScopeLock lock( &_lock );
		std::map<std::wstring, LPCONTROL_RECEIVEDATAFUNCTION_T>::iterator iter;
		for( iter=_callbackList.begin(); iter!=_callbackList.end(); iter++ )
		{
			delete (*iter).second;
		}	
		_callbackList.clear();
	}
	else
	{
	    wstring nodeName( pNodeName );
		CScopeLock lock( &_lock );
		std::map<std::wstring, LPCONTROL_RECEIVEDATAFUNCTION_T>::iterator iter = _callbackList.find( nodeName );
		if( iter!=_callbackList.end() )
		{
			delete (*iter).second;
		}
		_callbackList.erase( nodeName ); 
	}
}