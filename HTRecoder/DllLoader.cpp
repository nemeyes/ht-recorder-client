#include "Common.h"
#include "DllLoader.h"
#include "ScopeLock.h"
#include <Shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

DllLoader::DllLoader( VOID )
{
	InitializeCriticalSection( &_cs );
	_hLibrary = NULL;
}

DllLoader::~DllLoader( VOID )
{
	Unload();
	DeleteCriticalSection( &_cs );
}

BOOL DllLoader::LoadBy( LPCTSTR lpBasePath, LPCTSTR lpFileName, LPCTSTR lpFileExt )
{
	CString strPath;
#ifdef _DEBUG
	strPath.Format(L"%s\\%sD.%s", lpBasePath, lpFileName, lpFileExt);
	if (PathFileExists(strPath)) return Load( strPath );
#endif
	strPath.Format(L"%s\\%s.%s", lpBasePath, lpFileName, lpFileExt);
	return Load(strPath);
}

BOOL DllLoader::Load( LPCTSTR lpLibPath )
{
	CScopeLock lock( &_cs );
	if( IsLoaded() ) return TRUE;
	_hLibrary = LoadLibrary(lpLibPath);
	if( !_hLibrary ) 
	{
		DWORD error = ::GetLastError();
		return FALSE;
	}

	if( PathIsRelative(lpLibPath) )
	{
		TCHAR szFileName[MAX_PATH] = {0};
		if( GetModuleFileName(_hLibrary, szFileName, sizeof(szFileName)) <= 0 ) return FALSE;
		_strLibPath = szFileName;
	}
	else
	{
		_strLibPath = lpLibPath;
	}

	return TRUE;
}

VOID DllLoader::Unload( VOID )
{
	CScopeLock lock( &_cs );

	if( !IsLoaded() ) return;
	_strLibPath.Empty();
	_mapProcs.clear();
	FreeLibrary( _hLibrary );
	_hLibrary = NULL;
}

FARPROC DllLoader::GetProc(int nkey, LPCSTR lpProcName)
{
	CScopeLock lock( &_cs );

	if (!IsLoaded())
		return NULL;

	if (_mapProcs.find(nkey) != _mapProcs.end())
		return _mapProcs[nkey];

	_mapProcs[nkey] = GetProcAddress(_hLibrary, lpProcName);
	return _mapProcs[nkey];
}


CString DllLoader::GetModulePath( VOID )
{
	TCHAR tszPath[MAX_PATH] = {0};
	HMODULE hModule = GetModuleHandle();
	if (GetModuleFileName(hModule, tszPath, sizeof(tszPath)) <= 0)
	{
		if (GetModuleFileName(NULL, tszPath, sizeof(tszPath)) <= 0)
		{
			return L"";
		}
	}

	PathRemoveFileSpec(tszPath);
	return tszPath;
}
