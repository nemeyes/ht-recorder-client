#pragma once

#define SCOPE_LOCK_START(x)		{ CScopeLock lock(x);
#define SCOPE_TRY_LOCK_START(x) { CScopeLock lock(x, FALSE);
#define SCOPE_LOCK_END()		}

class CScopeLock
{
public:
	CScopeLock(CRITICAL_SECTION* pLock = NULL, BOOL bBlocking = TRUE);
	~CScopeLock(void);

	static void InitializeLock();
	static void DestoryLock();
	static LPVOID ExchangePointer(CRITICAL_SECTION* pLock, LPVOID* ppTarget, LPVOID pValue)
	{
		CScopeLock lock(pLock);
		LPVOID lpPrevValue = *ppTarget;
		*ppTarget = pValue;
		return lpPrevValue;
	};

protected:
	static BOOL bInit;
	static CRITICAL_SECTION cs;

	CRITICAL_SECTION *m_pLock;
};
