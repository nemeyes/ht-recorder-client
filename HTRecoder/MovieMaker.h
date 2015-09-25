#pragma once

#include "DllLoader.h"

enum MOVIE_FILE_FORMAT
{
	MOVIE_FILE_FORMAT_AVI = 0
};

#ifdef MOVMAKER_EXPORTS
#define MOVMAKER_API	__declspec(dllexport)
#else
#define MOVMAKER_API	__declspec(dllimport)
#endif

#define DEFCALL			__cdecl

typedef LONG_PTR MOVMAKER_HANDLE;

// Create handle for maker.
EXTERN_C MOVMAKER_API MOVMAKER_HANDLE MOVMAKER_Create(MOVIE_FILE_FORMAT eFormat);
typedef MOVMAKER_HANDLE (DEFCALL* LPMOVMAKER_Create)(MOVIE_FILE_FORMAT eFormat);

// Release handle.
EXTERN_C MOVMAKER_API void MOVMAKER_Close(MOVMAKER_HANDLE* ppHandle);
typedef void (DEFCALL* LPMOVMAKER_Close)(MOVMAKER_HANDLE* ppHandle);

// Initialize maker.
// Create movie file and open it for write
// [Return Value]
//		< 0 : Fail
//		= 0 : Success
EXTERN_C MOVMAKER_API int MOVMAKER_Start(MOVMAKER_HANDLE handle, LPCTSTR pszFilePath);
typedef int (DEFCALL* LPMOVMAKER_Start)(MOVMAKER_HANDLE handle, LPCTSTR pszFilePath);

// Uninitialize maker
// Close file and release resources.
// [Return Value]
//		< 0 : Fail
//		= 0 : Success
EXTERN_C MOVMAKER_API int MOVMAKER_Stop(MOVMAKER_HANDLE handle);
typedef int (DEFCALL* LPMOVMAKER_Stop)(MOVMAKER_HANDLE handle);

// Is started?
EXTERN_C MOVMAKER_API BOOL MOVMAKER_IsStarted(MOVMAKER_HANDLE handle);
typedef BOOL (DEFCALL* LPMOVMAKER_IsStarted)(MOVMAKER_HANDLE handle);

// Add stream data.
// Parameter: pStreamData type is LPStreamData (See LiveSession5 library)
// [Return Value]
//		< 0 : Fail
//		= 0 : Success
EXTERN_C MOVMAKER_API int MOVMAKER_AddStreamData(MOVMAKER_HANDLE handle, LPVOID pStreamData, int nDataSize);
typedef int (DEFCALL* LPMOVMAKER_AddStreamData)(MOVMAKER_HANDLE handle, LPVOID pStreamData, int nDataSize);

DEFINE_PROC_KEY_NAME(_CREATE,					0,	"MOVMAKER_Create")
DEFINE_PROC_KEY_NAME(_CLOSE,					1,	"MOVMAKER_Close")
DEFINE_PROC_KEY_NAME(_START,					50,	"MOVMAKER_Start")
DEFINE_PROC_KEY_NAME(_STOP,						51,	"MOVMAKER_Stop")
DEFINE_PROC_KEY_NAME(_ISSTARTED,				52,	"MOVMAKER_IsStarted")
DEFINE_PROC_KEY_NAME(_ADDSTREAMDATA,			100, "MOVMAKER_AddStreamData")



class MovieMaker : public DllLoader
{
public:
	MovieMaker( MOVIE_FILE_FORMAT eFormat );
	virtual ~MovieMaker( VOID );

	virtual BOOL	Load( LPCTSTR lpLibPath) override;
	virtual VOID	Unload( VOID ) override;

	MOVMAKER_HANDLE GetHandle( VOID ) { return _hMaker; };

	INT				Start( CString strFilePath );
	INT				Stop( VOID );
	BOOL			IsStarted( VOID );
	INT				AddStreamData( LPStreamData pData );
	
protected:
	MOVIE_FILE_FORMAT	_eMovieFormat;
	MOVMAKER_HANDLE		_hMaker;

	virtual BOOL	Create( VOID );
	virtual void	Close( VOID );
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MovieMaker
// 전달되는 Stream 정보를 이용하여 지정된 Movie 파일의 Format 으로 파일을 생성한다.
// 

