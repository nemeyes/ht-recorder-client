#pragma once
#include <map>
#include <ThreadPool.h>
#include <list>

#ifndef _MSGPARAM_DEFINED
#define _MSGPARAM_DEFINED
typedef UINT_PTR MSGPARAM;
#endif

#ifndef _REGPARAM_DEFINED
#define _REGPARAM_DEFINED
typedef LONG_PTR REGPARAM;
#endif

// pMsgParam : Parameter is dependent on message.
// pRegParam : Parameter is registered from listner.
typedef void (CALLBACK* LPMESSAGEHANDLER)(UINT uMessage, MSGPARAM pMsgParam, REGPARAM pRegParam);

class CMessageSender
{
protected:
	struct WORKER_ITEM
	{
		WORKER_ITEM()
		{
			pSender = NULL;
			uMessage = 0;
			pMsgParam = NULL;
		};

		WORKER_ITEM(CMessageSender* sender, UINT message, MSGPARAM pParam)
		{
			pSender = sender;
			uMessage = message;
			pMsgParam = pParam;
		};

		CMessageSender* pSender;
		UINT uMessage;
		MSGPARAM pMsgParam;
	};
public:
	CMessageSender(void);
	virtual ~CMessageSender(void);

	virtual BOOL RegisterHandler(LPMESSAGEHANDLER pHandler, REGPARAM lpParam);
	virtual void UnregisterHandler(LPMESSAGEHANDLER pHandler);

	void NotifyMessage(UINT uMessage, BOOL bAsync = FALSE)
	{
		NotifyMessage(uMessage, NULL, bAsync);
	};
	virtual void NotifyMessage(UINT uMessage, MSGPARAM pMsgParam, BOOL bAsync = FALSE);

	static void SetThreadPool(CSafeThreadPool* pThreadPool, size_t nWorkerId)
	{
		m_pThreadPool = pThreadPool;
		m_nWorkThreadId = nWorkerId;
	};

protected:
	std::map<LPMESSAGEHANDLER, REGPARAM> m_mapMessageHandlers;
	std::map<UINT, UINT> m_lockMessages;
	CRITICAL_SECTION m_csHandler;
	static CSafeThreadPool* m_pThreadPool;
	static size_t m_nWorkThreadId;
	CSafeThreadPool* m_pSelfThreadPool;
	BOOL m_bIsDestroy;
	static int m_nNextThreadJobKey;
	int m_nThreadJobKey;

	virtual void NotifyMessageCore(UINT uMessage, MSGPARAM pMsgParam);
	int GetHandlerCount() { return m_mapMessageHandlers.size(); };

	void LockMessage(UINT uMessage);
	void UnlockMessage(UINT uMessage);
	BOOL IsLocked(UINT uMessage);

	static void CALLBACK SendWorker(void* arg, size_t key);
};
