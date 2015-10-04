#pragma once

class CThreadUtil
{
protected:
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // must be 0x1000
		LPCSTR szName; // pointer to name (in user addr space)
		DWORD dwThreadID; // thread ID (-1=caller thread)
		DWORD dwFlags; // reserved for future use, must be zero
	} THREADNAME_INFO;

public:
	static void SetCurrentThreadName(LPCSTR szThreadName)
	{
		SetThreadName(GetCurrentThreadId(), szThreadName);
	};
	static void SetThreadName(DWORD dwThreadID, LPCSTR szThreadName);

};