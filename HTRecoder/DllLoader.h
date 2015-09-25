#pragma once
#include <map>

#define DEFINE_PROC_KEY_NAME(name, key, procName)						\
	const int PROC_KEY##name				= key;						\
	const LPCSTR PROC_NAME##name			= procName;

#define PROC_KEY(name)						PROC_KEY##name
#define PROC_NAME(name)						PROC_NAME##name
#define PROC_KEY_NAME(name)					PROC_KEY(name), PROC_NAME(name)

#define START_BREAKABLE_ROUTINE()			BOOL _IsFailed = FALSE;		\
	do {
#define END_BREAKABLE_ROUTINE()				} while(0);
#define BREAK_ROUTINE(bIsFail)				_IsFailed = bIsFail;		\
	break;
#define IS_FAILED_ROUTINE()					_IsFailed				


class DllLoader
{
public:
	DllLoader( VOID );
	virtual ~DllLoader( VOID );

	virtual BOOL	LoadBy( LPCTSTR lpBasePath, LPCTSTR lpFileName, LPCTSTR lpFileExt );
	virtual BOOL	Load( LPCTSTR lpLibPath );
	virtual VOID	Unload( VOID );

	BOOL			IsLoaded( VOID ) { return (_hLibrary != NULL); };
	CString			GetLibraryPath( VOID ) { return _strLibPath; };


protected:
	HMODULE					_hLibrary;
	std::map<INT, FARPROC>	_mapProcs;
	CRITICAL_SECTION		_cs;
	CString					_strLibPath;

	FARPROC					GetProc( INT nKey, LPCSTR lpProcName );
	virtual HMODULE			GetModuleHandle( VOID )
	{
		return reinterpret_cast<HMODULE>(&__ImageBase);
	};
	CString					GetModulePath( VOID );
};