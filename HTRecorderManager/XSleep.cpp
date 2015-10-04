#include "stdafx.h"

// This structure is used internally by the XSleep function 
struct XSleep_Structure
{
	int duration;
	HANDLE eventHandle;
};


//////////////////////////////////////////////////////////////////////
// Function  : XSleepThread()
// Purpose   : The thread which will sleep for the given duration
// Returns   : DWORD WINAPI
// Parameters:       
//  1. pWaitTime -
//////////////////////////////////////////////////////////////////////
DWORD WINAPI XSleepThread(LPVOID pWaitTime)
{
	XSleep_Structure *sleep = (XSleep_Structure *)pWaitTime;

	Sleep(sleep->duration);
	SetEvent(sleep->eventHandle);

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Function  : XSleep()
// Purpose   : To make the application sleep for the specified time
//			   duration.
//			   Duration the entire time duration XSleep sleeps, it
//			   keeps processing the message pump, to ensure that all
//			   messages are posted and that the calling thread does
//			   not appear to block all threads!
// Returns   : none
// Parameters:       
//  1. nWaitInMSecs - Duration to sleep specified in miliseconds.
//////////////////////////////////////////////////////////////////////
void XSleep(int nWaitInMSecs)
{
	XSleep_Structure sleep;

	sleep.duration = nWaitInMSecs;
	sleep.eventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);

	DWORD threadId;
	CreateThread(NULL, 0, &XSleepThread, &sleep, 0, &threadId);

	MSG msg;

	while (::WaitForSingleObject(sleep.eventHandle, 0) == WAIT_TIMEOUT)
	{
		//get and dispatch messages
		if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	CloseHandle(sleep.eventHandle);
}

