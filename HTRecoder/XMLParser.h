#pragma once

class ServiceCore;
typedef VOID (CALLBACK* LPCONTROL_RECEIVEDATAFUNCTION)( CONST VOID* pData, size_t nDataSize, VOID* pUserContext );
typedef struct _CONTROL_RECEIVEDATAFUNCTION_T
{
	_CONTROL_RECEIVEDATAFUNCTION_T( VOID ) 
		: func(NULL)
		, base(NULL)
		, pUserContext(NULL) 
	{
	
	}
	LPCONTROL_RECEIVEDATAFUNCTION	func;
	VOID							*pUserContext;
	__timeb64						t;
	ServiceCore						*base;
} CONTROL_RECEIVEDATAFUNCTION_T, *LPCONTROL_RECEIVEDATAFUNCTION_T;


typedef struct _QUEUE_WORKER_ELEMENT_T
{
	CHAR							*pXML;
	UINT							nSize;
	LPCONTROL_RECEIVEDATAFUNCTION_T f;
} QUEUE_WORKER_ELEMENT_T, *LPQUEUE_WORKER_ELEMENT_T;

class XMLParser
{
public:
	typedef struct _RS_STREAM_INFO_T
	{
		_RS_STREAM_INFO_T( VOID ) 
			: pStreamHandle(NULL) {}
		size_t	nId;
		CString	strMAC;
		CString	strProfile;
		CString	strAddress;
		CString	strModel;
		CString strModelType;
		CString strUrl;

		UINT	nRtspPort;
		UINT	nHttpPort;
		UINT	nHttpsPort;
		BOOL	bSSL;
		VOID*	pStreamHandle;

	} RS_STREAM_INFO_T;

	XMLParser( ServiceCore* service );
	virtual ~XMLParser( VOID );

	INT32 Process( CONST CHAR *pXML, size_t nSize );

	INT32 GetClientId( VOID ) CONST { return _clientId; }
	VOID ClearStreamListAll( VOID );
	BOOL IsEqualReqId( CString& strReqId );
	CString GetReqId( VOID ) CONST { return _reqId; }

	VOID AddNotifyCallback( LPCTSTR pNodeName, LPCONTROL_RECEIVEDATAFUNCTION Func, VOID* pUserContext );
	VOID RemoveNotifyCallback( LPCTSTR pNodeName );


	static VOID CALLBACK OnNotification( VOID *arg, size_t key );
	VOID ProcessNotification( size_t key );
	//static VOID CALLBACK ProcessNotification( VOID *arg, size_t key );

protected:
	INT32 _ProcessXML_GetClientIDRes( MSXML2::IXMLDOMNodePtr pNode );
	INT32 _ProcessXML_GetAllStreamInfoRes( MSXML2::IXMLDOMNodeListPtr pList );
	INT32 _ProcessXML_LoadMSConfigRes( MSXML2::IXMLDOMNodeListPtr pList );

	int														_clientId;
	CString													_reqId;
	ServiceCore												*_service;
	std::multimap<wstring, RS_STREAM_INFO_T*>				_streamMultiMap;
	std::map<std::wstring, LPCONTROL_RECEIVEDATAFUNCTION_T>	_callbackList;
	CRITICAL_SECTION										_lock;

	std::queue<LPQUEUE_WORKER_ELEMENT_T>					_queue;
};