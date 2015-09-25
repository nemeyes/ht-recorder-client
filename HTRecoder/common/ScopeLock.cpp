#include "Common.h"
#include "ScopeLock.h"

BOOL CScopeLock::bInit = FALSE;
CRITICAL_SECTION CScopeLock::cs;

CScopeLock::CScopeLock(CRITICAL_SECTION *pLock /*= NULL*/, BOOL bBlocking/* = TRUE*/)
:m_pLock(pLock)
{
	if (bBlocking)
	{
		if (m_pLock)
			EnterCriticalSection(m_pLock);
		else
			EnterCriticalSection(&cs);
	}
	else
	{
		BOOL bEnter = FALSE;
		do 
		{
			if (m_pLock)
				bEnter = TryEnterCriticalSection(m_pLock);
			else
				bEnter = TryEnterCriticalSection(&cs);

			if (bEnter)
				break;

			Sleep(1);
		} while(!bEnter);
	}
}

CScopeLock::~CScopeLock(void)
{
	if (m_pLock)
		LeaveCriticalSection(m_pLock);
	else
		LeaveCriticalSection(&cs);
}

void CScopeLock::InitializeLock()
{
	if(!bInit)
	{
		InitializeCriticalSection(&cs);
		bInit = TRUE;
	}
}

void CScopeLock::DestoryLock()
{
	if(bInit)
	{
		DeleteCriticalSection(&cs);
		bInit = FALSE;
	}
}