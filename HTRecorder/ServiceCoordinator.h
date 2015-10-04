#pragma once
#include <Guiddef.h>

class CSafeThreadPool;
class ServiceCoordinator
{
public:
	static ServiceCoordinator& Instance( VOID );
	CStringW			GetGUIDW( VOID );
	CStringA			GetGUIDA( VOID );
	UINT				GetServerID( VOID );

	CSafeThreadPool *	GetThreadPool( VOID );

	BOOL ConvertWide2MultiByteCharacter( WCHAR *source, char **destination );
	BOOL ConvertMultiByte2WideCharacter( char *source, WCHAR **destination );

private:
	ServiceCoordinator( VOID );
	~ServiceCoordinator( VOID );

private:
	GUID					*_guid;
	static CSafeThreadPool	_threadPool;
	UINT					_serverId;

};