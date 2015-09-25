#include "Common.h"
#include "ThreadPool.h"
#include <process.h>
#include "ThreadUtil.h"
#include <strsafe.h>
#include "ScopeLock.h"
#pragma comment(lib, "strsafe.lib")

#define DEFAULT_THREAD_POOL_NAME		"ThreadPool"

ThreadPool::ThreadPool(void)
:m_nNumberOfThread(0)
,m_pWorker(NULL)
,m_lWorker(0)
,m_bCheckJobKey(FALSE)
{
	InitializeCriticalSection(&m_cs);
}

ThreadPool::~ThreadPool(void)
{
	Destroy();
	DeleteCriticalSection(&m_cs);
}

unsigned int WINAPI ThreadPool::_WorkerThreadFunction(LPVOID arg)
{
	worker_t *pWorker = reinterpret_cast<worker_t*>(arg);
	ThreadPool *pTP = pWorker->pThreadPool;
	function_t func;

	while(pTP->m_bRun)
	{
		if(pTP->GetWorkerFunction(func))
		{
			if (pTP->m_bCheckJobKey)
			{
				do 
				{
					try
					{
						func.function(func.arg, func.key);
					}
					catch(...)
					{
						TRACE(L"[ThreadPool] ERROR - Exception raised in worker function\n");
					}
				} while (pTP->m_bRun && !pTP->PopWorkerFunction(&func.id));
			}
			else
			{
				try
				{
					func.function(func.arg, func.key);
				}
				catch(...)
				{
					TRACE(L"[ThreadPool] ERROR - Exception raised in worker function\n");
				}
			}
		}
		else
		{
			InterlockedDecrement(&pTP->m_lWorker);
			InterlockedExchange(&pWorker->bThreadRun, FALSE);

			//pWorker->bThreadRun = FALSE;
			WaitForSingleObject(pWorker->hEvent, INFINITE);
		}
	}

	return 0;
}

BOOL ThreadPool::Initialize(LPCSTR lpstrName/* = NULL*/, int nNumberOfThread /*= 0*/, int nPriority /*= THREAD_PRIORITY_NORMAL*/)
{
	if(nNumberOfThread == 0)
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		m_nNumberOfThread = si.dwNumberOfProcessors * 2;
	}
	else
		m_nNumberOfThread = nNumberOfThread;

	m_pWorker = new worker_t[m_nNumberOfThread];
	m_bRun = TRUE;

	InterlockedExchange(&m_lWorker, m_nNumberOfThread);

	for(int i=0; i<m_nNumberOfThread; ++i)
	{
		m_pWorker[i].pThreadPool	= this;
		m_pWorker[i].nIndex			= i;
		m_pWorker[i].bThreadRun		= FALSE;
		m_pWorker[i].hEvent			= CreateEvent(NULL, FALSE, FALSE, NULL);
		m_pWorker[i].hThread		= (HANDLE)_beginthreadex(NULL, 0, _WorkerThreadFunction, &m_pWorker[i], NULL, (unsigned int*)&m_pWorker[i].dwThreadId);
		CThreadUtil::SetThreadName(m_pWorker[i].dwThreadId, lpstrName ? lpstrName : DEFAULT_THREAD_POOL_NAME);

		SetThreadPriority(m_pWorker[i].hThread, nPriority);

		WaitForSingleObject(m_pWorker[i].hEvent, 10);
	}

	return TRUE;
}

void ThreadPool::Destroy()
{
	if(!m_pWorker) return;

	EnterCriticalSection(&m_cs);
	m_lstFunctions.clear();
	LeaveCriticalSection(&m_cs);

	m_bRun = FALSE;
	Sleep(500);
	DWORD dwWait;
	for(int i=0; i<m_nNumberOfThread; ++i)
	{
		SetEvent(m_pWorker[i].hEvent);
		dwWait = WaitForSingleObject(m_pWorker[i].hThread, 1000);
		if(dwWait == WAIT_TIMEOUT)
		{
			TRACE(">>> 쓰레드 타임아웃이네.... %d \n", m_lstFunctions.size());
			DWORD dwExitCode;
			GetExitCodeThread(m_pWorker[i].hThread, &dwExitCode);
			TerminateThread(m_pWorker[i].hThread, dwExitCode);
		}
		CloseHandle(m_pWorker[i].hThread);
		CloseHandle(m_pWorker[i].hEvent);
	}
	delete [] m_pWorker;
	m_pWorker = NULL;
}

void ThreadPool::AddWorkerFunction(WORKERFUNCTION worker, LPVOID arg, size_t nKey/* = 0*/)
{
	CScopeLock lock(&m_cs);

	if(!m_pWorker) return;

	if(m_lWorker < m_nNumberOfThread)
	{
		for(int i=0; i<m_nNumberOfThread; ++i)
			if(!m_pWorker[i].bThreadRun)
			{
				//m_pWorker[i].bThreadRun = TRUE;
				InterlockedExchange(&m_pWorker[i].bThreadRun, TRUE);
				InterlockedIncrement(&m_lWorker);
				SetEvent(m_pWorker[i].hEvent);
				break;
			}
	}

	if (m_bCheckJobKey)
	{
		std::list<function_t>::iterator pos = m_lstFunctions.begin();
		while(pos != m_lstFunctions.end())
		{
			if (((*pos).key == nKey) && ((*pos).function == worker))
			{
				(*pos).nReqCallCount++;
				return;
			}
			pos++;
		}
	}

	function_t func;
	if (m_bCheckJobKey)
		CoCreateGuid(&func.id);
	func.function = worker;
	func.arg = arg;
	func.key = nKey;
	func.bIsProcessing = FALSE;
	func.nReqCallCount = 1;
	m_lstFunctions.push_back(func);
}

int ThreadPool::GetJobCount()
{
	CScopeLock lock(&m_cs);

	return m_lstFunctions.size();
}

BOOL ThreadPool::GetWorkerFunction(function_t &func)
{
	BOOL bResult = FALSE;
	EnterCriticalSection(&m_cs);
	std::list<function_t>::iterator pos = m_lstFunctions.begin();
	while(pos != m_lstFunctions.end())
	{
		if (!(*pos).bIsProcessing)
		{
			(*pos).bIsProcessing = TRUE;
			func = (*pos);
			bResult = TRUE;
			if (!m_bCheckJobKey)
				m_lstFunctions.erase(pos);
			break;
		}
		pos++;
	}
	LeaveCriticalSection(&m_cs);

	return bResult;
}

// m_bCheckJobKey 가 TRUE 일 때만 호출되어지는 Function
BOOL ThreadPool::PopWorkerFunction(LPGUID pGuid)
{
	BOOL bResult = TRUE;

	EnterCriticalSection(&m_cs);
	std::list<function_t>::iterator pos = m_lstFunctions.begin();
	while(pos != m_lstFunctions.end())
	{
		if ((*pos).id != (*pGuid))
		{
			pos++;
			continue;
		}

		(*pos).nReqCallCount--;
		if ((*pos).nReqCallCount > 0)
		{
			bResult = FALSE;
			break;
		}

		m_lstFunctions.erase(pos);
		break;
	}
	LeaveCriticalSection(&m_cs);
	return bResult;
}

BOOL ThreadPool::IsInitialize()
{
	return (m_pWorker != NULL);
}

void ThreadPool::ClearAllWorkerFunction()
{
	EnterCriticalSection(&m_cs);
	m_lstFunctions.clear();
	LeaveCriticalSection(&m_cs);
}

#define DEFAULT_SAFE_THREAD_POOL_NAME		"SafeThreadPool"

/////////////////////////////////////////////////////////////////////////////////////////
// CSafeThreadPool
CSafeThreadPool::CSafeThreadPool()
{
	OnConstructor(4096, FALSE);
}

CSafeThreadPool::CSafeThreadPool(BOOL bAutoCreate)
{
	OnConstructor(4096, bAutoCreate);
}

CSafeThreadPool::CSafeThreadPool(size_t nMaxQueueSize)
{
	OnConstructor(nMaxQueueSize, FALSE);
}

CSafeThreadPool::CSafeThreadPool(size_t nMaxQueueSize, BOOL bAutoCreate)
{
	OnConstructor(nMaxQueueSize, bAutoCreate);
}

void CSafeThreadPool::OnConstructor(size_t nMaxQueueSize, BOOL bAutoCreate)
{
	InitializeCriticalSection(&m_csPool);
	m_nNumberOfThread = 0;
	m_pWorker = NULL;
	m_lWorker = 0;
	m_nMaxQueueSize = nMaxQueueSize;
	m_bAutoCreate = bAutoCreate;
	m_bRun = FALSE;
	m_nPriority = THREAD_PRIORITY_NORMAL;
	ZeroMemory(&m_szPoolName, sizeof(m_szPoolName));
}

CSafeThreadPool::~CSafeThreadPool()
{
	Destroy();
	DeleteCriticalSection(&m_csPool);
}

int CSafeThreadPool::GetDefaultSize()
{
	SYSTEM_INFO si = {0};
	GetSystemInfo(&si);
	return (si.dwNumberOfProcessors * 2);
}


void CSafeThreadPool::SetWorkerName(UINT nIndex, LPCSTR lpstrName)
{
	CScopeLock lock(&m_csPool);
	if ((nIndex < 0) || (nIndex >= m_nNumberOfThread))
		return;

	if (!m_pWorker[nIndex].hThread)
		return;

	CThreadUtil::SetThreadName(m_pWorker[nIndex].dwThreadId, lpstrName);
}

BOOL CSafeThreadPool::Initialize(LPCSTR lpstrName/* = NULL*/, size_t nNumberOfThread /*= 0*/, int nPriority /*= THREAD_PRIORITY_NORMAL*/)
{
	CScopeLock lock(&m_csPool);
	if (m_bRun)
		return FALSE;

	if(nNumberOfThread == 0)
	{
		m_nNumberOfThread = GetDefaultSize();
	}
	else
		m_nNumberOfThread = nNumberOfThread;


	m_bRun = TRUE;
	ZeroMemory(&m_szPoolName, sizeof(m_szPoolName));
	if(lpstrName)
	        StringCchCopyA(m_szPoolName, sizeof(m_szPoolName), lpstrName);
	m_nPriority = nPriority;
	m_pWorker = new worker_t[m_nNumberOfThread];
	
	for(size_t i=0; i<m_nNumberOfThread; ++i)
	{
		InitializeCriticalSection(&m_pWorker[i].cs);
		m_pWorker[i].pThreadPool	= this;
		m_pWorker[i].nIndex			= i;
		m_pWorker[i].bThreadRun		= TRUE;
		m_pWorker[i].hEvent			= CreateEvent(NULL, FALSE, FALSE, NULL);
		if (!m_bAutoCreate)
		{
			CreateWorker(i);
		}
	}

	return TRUE;
}


void CSafeThreadPool::CreateWorker(int nWorkerIndex)
{
	m_pWorker[nWorkerIndex].hThread	= (HANDLE)_beginthreadex(NULL, 0, _WorkerThreadFunction, &m_pWorker[nWorkerIndex], NULL, (unsigned int*)&m_pWorker[nWorkerIndex].dwThreadId);
	if (!m_pWorker[nWorkerIndex].hThread)
		return;

	CThreadUtil::SetThreadName(m_pWorker[nWorkerIndex].dwThreadId, (lstrlenA(m_szPoolName) > 0) ? m_szPoolName : DEFAULT_SAFE_THREAD_POOL_NAME);
	SetThreadPriority(m_pWorker[nWorkerIndex].hThread, m_nPriority);
}

void CSafeThreadPool::Destroy()
{
	CScopeLock lock(&m_csPool);
	if(!m_pWorker) return;

	m_bRun = FALSE;
	DWORD dwWait;
	for(size_t i=0; i<m_nNumberOfThread; i++)
	{
		EnterCriticalSection(&m_pWorker[i].cs);
		m_pWorker[i].qFunctions.RemoveAll();

		m_pWorker[i].bThreadRun = FALSE;
		SetEvent(m_pWorker[i].hEvent);
		if (m_pWorker[i].hThread)
		{
			DWORD dwExitCode;
			GetExitCodeThread(m_pWorker[i].hThread, &dwExitCode);
			if (dwExitCode == STILL_ACTIVE)
			{
				dwWait = WaitForSingleObject(m_pWorker[i].hThread, 1000);
				if(dwWait == WAIT_TIMEOUT)
				{
					TRACE("CSafeThreadPool 타임아웃: %d\n", i);
					TerminateThread(m_pWorker[i].hThread, 0);
				}
			}
			CloseHandle(m_pWorker[i].hThread);
		}
		CloseHandle(m_pWorker[i].hEvent);

		DeleteCriticalSection(&m_pWorker[i].cs);
	}
	delete [] m_pWorker;
	m_pWorker = NULL;
}

BOOL CSafeThreadPool::AddWorkerFunction(WORKERFUNCTION worker, LPVOID arg, size_t nID, size_t nKey)
{
	CScopeLock lock(&m_csPool);
	if(!m_pWorker || !m_bRun) return FALSE;

	size_t n = 0;
	if (nID < 0)
	{
		int nMinCount = -1;
		for( UINT nLoopCnt = 0; nLoopCnt < m_nNumberOfThread; nLoopCnt++)
		{
			EnterCriticalSection(&m_pWorker[nLoopCnt].cs);
			if ((nMinCount < 0) || (nMinCount > m_pWorker[nLoopCnt].qFunctions.GetCount()))
			{
				nMinCount = m_pWorker[nLoopCnt].qFunctions.GetCount();
				n = nLoopCnt;
			}
			LeaveCriticalSection(&m_pWorker[nLoopCnt].cs);

		}
	}
	else
	{
		n = nID % m_nNumberOfThread;
	}
	function_t func;
	func.function	= worker;
	func.arg		= arg;
	func.key		= nKey;

	EnterCriticalSection(&m_pWorker[n].cs);
	if (m_bAutoCreate)
	{
		if (m_pWorker[n].hThread)
		{
			DWORD dwExitCode = 0;
			GetExitCodeThread(m_pWorker[n].hThread, &dwExitCode);
			if (dwExitCode != STILL_ACTIVE)
			{
				CloseHandle(m_pWorker[n].hThread);
				m_pWorker[n].hThread = NULL;
				CreateWorker(n);
			}
		}
		else
		{
			CreateWorker(n);
		}
	}
	/*if(m_pWorker[n].qFunctions.GetCount() > m_nMaxQueueSize)
	{
		LeaveCriticalSection(&m_pWorker[n].cs);
		return FALSE;
	}*/
	m_pWorker[n].qFunctions.AddTail(func);
	LeaveCriticalSection(&m_pWorker[n].cs);

	return TRUE;
}

void CSafeThreadPool::ClearAllWorkerFunction()
{
	CScopeLock lock(&m_csPool);
	if(!m_pWorker) return;

	for(size_t i=0; i<m_nNumberOfThread; i++)
	{
		EnterCriticalSection(&m_pWorker[i].cs);
		m_pWorker[i].qFunctions.RemoveAll();
		LeaveCriticalSection(&m_pWorker[i].cs);
	}
}

void CSafeThreadPool::ClearWorkerFunction(size_t nID, size_t nKey)
{
	CScopeLock lock(&m_csPool);
	if(!m_pWorker) return;

	size_t n = nID % m_nNumberOfThread;
	POSITION pos, temp;
	function_t func;


	EnterCriticalSection(&m_pWorker[n].cs);

	pos = m_pWorker[n].qFunctions.GetHeadPosition();
	while(pos != NULL)
	{
		func = m_pWorker[n].qFunctions.GetAt(pos);
		if(func.key == nKey)
		{
			temp = pos;
			m_pWorker[n].qFunctions.GetNext(pos);
			m_pWorker[n].qFunctions.RemoveAt(temp);
		}
		else
			m_pWorker[n].qFunctions.GetNext(pos);
	}
	
	LeaveCriticalSection(&m_pWorker[n].cs);
}

BOOL CSafeThreadPool::IsInitialize()
{
	return (m_pWorker != NULL);
}

unsigned int WINAPI CSafeThreadPool::_WorkerThreadFunction(LPVOID arg)
{
	worker_t *pWorker = reinterpret_cast<worker_t*>(arg);
	CSafeThreadPool *pTP = pWorker->pThreadPool;
	function_t func;


	while(pWorker->bThreadRun)
	{
		EnterCriticalSection(&pWorker->cs);
		if(pWorker->qFunctions.IsEmpty())
		{
			LeaveCriticalSection(&pWorker->cs);
			if (pTP->m_bAutoCreate)
				break;
			
			//WaitForSingleObject(pWorker->hEvent, INFINITE);
			Sleep(1);

			continue;
		}

		func = pWorker->qFunctions.RemoveHead();
		LeaveCriticalSection(&pWorker->cs);
		
		try
		{
			func.function( func.arg, func.key );
		}
		catch(...)
		{
			TRACE(L"[CSafeThreadPool] ERROR - Exception raised in worker function.\n");
		}
	}

	TRACE("CSafeThreadPool 종료 [%d]\n", pWorker->nIndex);

	return 0;
}