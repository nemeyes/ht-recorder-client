#include "Common.h"
#include <ThreadPool.h>
#include "ServiceCoordinator.h"
#include "common/ThreadPool.h"
#pragma comment(lib, "Rpcrt4.lib")

CSafeThreadPool	ServiceCoordinator::_threadPool;

ServiceCoordinator::ServiceCoordinator( VOID )
{
	_threadPool.Initialize( "LiveThreadPool", 2, THREAD_PRIORITY_ABOVE_NORMAL );
	_threadPool.SetWorkerName( 0, "MessageSenderWorker" );
	CMessageSender::SetThreadPool( &_threadPool, 0 );
	_serverId = 1;
	_guid = new GUID();
}

ServiceCoordinator::~ServiceCoordinator( VOID )
{
	if( _guid )
	{
		delete _guid;
		_guid = NULL;
	}
	//_threadPool.Destroy();
	//delete _threadPool;
}


BOOL ServiceCoordinator::ConvertWide2MultiByteCharacter( WCHAR *source, char **destination )
{
	UINT32 len = WideCharToMultiByte( CP_ACP, 0, source, wcslen(source), NULL, NULL, NULL, NULL );
	(*destination) = new char[NULL, len+1];
	::ZeroMemory( (*destination), (len+1)*sizeof(char) );
	WideCharToMultiByte( CP_ACP, 0, source, -1, (*destination), len, NULL, NULL );
	return TRUE;
}

BOOL ServiceCoordinator::ConvertMultiByte2WideCharacter( char *source, WCHAR **destination )
{
	UINT32 len = MultiByteToWideChar( CP_ACP, 0, source, strlen(source), NULL, NULL );
	(*destination) = SysAllocStringLen( NULL, len+1 );
	::ZeroMemory( (*destination), (len+1)*sizeof(WCHAR) );
	MultiByteToWideChar( CP_ACP, 0, source, -1, (*destination), len );
	
	return TRUE;
}

ServiceCoordinator& ServiceCoordinator::Instance( VOID )
{
	static ServiceCoordinator _instance;
	return _instance;
}

CStringW ServiceCoordinator::GetGUIDW( VOID )
{
	CStringW strGUID;
	CoCreateGuid( _guid );
	RPC_WSTR guid;
	UuidToStringW( _guid, &guid );
	strGUID.Format( _T("urn:uuid:%s"), guid );
	RpcStringFreeW( &guid );
	return strGUID;
}

CStringA ServiceCoordinator::GetGUIDA( VOID )
{
	CStringA strGUID;
	CoCreateGuid( _guid );
	RPC_CSTR guid;
	UuidToStringA( _guid, &guid );
	strGUID.Format( "urn:uuid:%s", guid );
	RpcStringFreeA( &guid );
	return strGUID;
}

CSafeThreadPool* ServiceCoordinator::GetThreadPool( VOID )
{
	return &_threadPool;
}

UINT ServiceCoordinator::GetServerID( VOID )
{
	UINT serverId = _serverId;
	_serverId++;
	return serverId;
}
