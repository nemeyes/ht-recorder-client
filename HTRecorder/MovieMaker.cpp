#include "Common.h"
#include "MovieMaker.h"
#include "LiveSession5.h"
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")


MovieMaker::MovieMaker(MOVIE_FILE_FORMAT eFormat) 
	: DllLoader()
	, _eMovieFormat(eFormat)
{
	_hMaker = NULL;
}

MovieMaker::~MovieMaker( VOID )
{
	Unload();
}

BOOL MovieMaker::Load(LPCTSTR lpLibPath)
{
	BOOL bLoad = DllLoader::Load(lpLibPath);

	if (bLoad)
	{
		if (!Create())
		{
			Unload();
			bLoad = FALSE;
		}
	}

	return bLoad;
}

void MovieMaker::Unload()
{
	Close();
	DllLoader::Unload();
}



BOOL MovieMaker::Create()
{
	if (_hMaker)
		return TRUE;

	LPMOVMAKER_Create pProc = (LPMOVMAKER_Create)GetProc(PROC_KEY_NAME(_CREATE));
	if (!pProc)
		return FALSE;

	_hMaker = pProc(_eMovieFormat);
	return (_hMaker != NULL);
}

void MovieMaker::Close()
{
	if (!_hMaker)
		return;

	LPMOVMAKER_Close pProc = (LPMOVMAKER_Close)GetProc(PROC_KEY_NAME(_CLOSE));
	if (!pProc)
		return;

	pProc(&_hMaker);
	_hMaker = NULL;
}


int MovieMaker::Start(CString strFilePath)
{
	if (!_hMaker)
		return -1;

	LPMOVMAKER_Start pProc = (LPMOVMAKER_Start)GetProc(PROC_KEY_NAME(_START));
	if (!pProc)
		return -2;

	return pProc(_hMaker, strFilePath);
}

int MovieMaker::Stop()
{
	if (!_hMaker)
		return -1;

	LPMOVMAKER_Stop pProc = (LPMOVMAKER_Stop)GetProc(PROC_KEY_NAME(_STOP));
	if (!pProc)
		return -2;

	return pProc(_hMaker);
}

BOOL MovieMaker::IsStarted()
{
	if (!_hMaker)
		return FALSE;

	LPMOVMAKER_IsStarted pProc = (LPMOVMAKER_IsStarted)GetProc(PROC_KEY_NAME(_ISSTARTED));
	if (!pProc)
		return FALSE;

	return pProc(_hMaker);
}

int MovieMaker::AddStreamData(LPStreamData pData)
{
	if (!_hMaker)
		return -1;

	LPMOVMAKER_AddStreamData pProc = (LPMOVMAKER_AddStreamData)GetProc(PROC_KEY_NAME(_ADDSTREAMDATA));
	if (!pProc)
		return -2;

	return pProc(_hMaker, pData, sizeof(StreamData));
}
