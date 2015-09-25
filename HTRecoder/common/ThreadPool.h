#pragma once
#include <queue>
#include <list>
#include <atlcoll.h>
using namespace std;

typedef void (__stdcall *WORKERFUNCTION)(LPVOID arg, size_t key);

class ThreadPool
{
public:
	typedef struct _worker_t
	{
		ThreadPool *pThreadPool;
		int nIndex;
		//BOOL bThreadRun;
		LONG bThreadRun;
		HANDLE hEvent;
		HANDLE hThread;
		DWORD dwThreadId;
	} worker_t;

	typedef struct _function_param_t
	{
		LPVOID	arg;
		size_t	nKey;
	} function_param_t;

	typedef struct _function_t
	{
		GUID id;
		WORKERFUNCTION function;
		LPVOID arg;
		size_t key;
		BOOL bIsProcessing;
		// m_bCheckJobKey 가 Enable 되어 있을 경우에만 이용
		// Thread 가 Job 을 처리하는 동안 추가 처리 요청이 발생하였을 경우 증가
		int nReqCallCount;
	} function_t;

	ThreadPool(void);
	~ThreadPool(void);

	BOOL Initialize(LPCSTR lpstrName = NULL, int nNumberOfThread = 0, int nPriority = THREAD_PRIORITY_NORMAL);
	void Destroy();
	void AddWorkerFunction(WORKERFUNCTION worker, LPVOID arg, size_t nKey = 0);
	void ClearAllWorkerFunction();
	int GetJobCount();

	BOOL IsInitialize();

	int GetNumberOfThread() const { return m_nNumberOfThread; }

	// Job Queue 에 Unique 한 Job 만 추가할 수 있도록 할 것인지 유무 (Job Key 이용)
	void EnableCheckJobKey(BOOL bEnable) { m_bCheckJobKey = bEnable; };
	BOOL IsCheckJobKey() { return m_bCheckJobKey; };

protected:
	static unsigned int WINAPI _WorkerThreadFunction(LPVOID arg);
	BOOL GetWorkerFunction(function_t &func);
	BOOL PopWorkerFunction(LPGUID pGuid);

	BOOL m_bRun;
	int m_nNumberOfThread;
	worker_t *m_pWorker;
	LONG m_lWorker;
	BOOL m_bCheckJobKey;

	CRITICAL_SECTION m_cs;
	list<function_t> m_lstFunctions;
};

class CSafeThreadPool
{
public:
	typedef struct _function_t
	{
		WORKERFUNCTION function;
		LPVOID arg;
		size_t key;
	} function_t;

	typedef struct _worker_t
	{
		CSafeThreadPool *pThreadPool;
		int nIndex;
		//BOOL bThreadRun;
		LONG bThreadRun;
		HANDLE hEvent;
		HANDLE hThread;
		CRITICAL_SECTION cs;
		CAtlList<function_t> qFunctions;
		DWORD dwThreadId;
	} worker_t;

	CSafeThreadPool();
	CSafeThreadPool(size_t nMaxQueueSize, BOOL bAutoCreate);
	CSafeThreadPool(BOOL bAutoCreate);
	CSafeThreadPool(size_t nMaxQueueSize);
	~CSafeThreadPool(void);

	BOOL Initialize(LPCSTR lpstrName = NULL, size_t nNumberOfThread = 0, int nPriority = THREAD_PRIORITY_NORMAL);
	void Destroy();
	BOOL AddWorkerFunction(WORKERFUNCTION worker, LPVOID arg, size_t nID, size_t nKey);
	void ClearAllWorkerFunction();
	void ClearWorkerFunction(size_t nID, size_t nKey);
	BOOL IsInitialize();
	size_t GetNumberOfThread() const { return m_nNumberOfThread; }
	void SetWorkerName(UINT nIndex, LPCSTR lpstrName);
	static int GetDefaultSize();

protected:
	void OnConstructor(size_t nMaxQueueSize, BOOL bAutoCreate);
	static unsigned int WINAPI _WorkerThreadFunction(LPVOID arg);
	BOOL GetWorkerFunction(function_t &func);
	void CreateWorker(int nWorkerIndex);

	BOOL m_bRun;
	size_t m_nNumberOfThread;
	worker_t *m_pWorker;
	LONG m_lWorker;
	size_t m_nMaxQueueSize;
	BOOL m_bAutoCreate;
	CRITICAL_SECTION m_csPool;
	char m_szPoolName[128];
	int m_nPriority;
};
