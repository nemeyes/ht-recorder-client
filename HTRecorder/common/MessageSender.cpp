#include "Common.h"
#include "MessageSender.h"
#include "ScopeLock.h"

CSafeThreadPool* CMessageSender::m_pThreadPool = NULL;
size_t CMessageSender::m_nWorkThreadId = 0;
int CMessageSender::m_nNextThreadJobKey = 0;

CMessageSender::CMessageSender(void)
{
	InitializeCriticalSection(&m_csHandler);
	m_bIsDestroy = FALSE;
	m_pSelfThreadPool = NULL;
	// m_nNextThreadJobKey 값은 계속 증가하지만, 제한값까지 증가되는 경우는 없을 것으로 보고
	// Reset 하는 과정은 고려하지 않는다.
	m_nThreadJobKey = m_nNextThreadJobKey++;
}

CMessageSender::~CMessageSender(void)
{
	m_bIsDestroy = TRUE;
	if (m_pThreadPool)
		m_pThreadPool->ClearWorkerFunction(m_nWorkThreadId, m_nThreadJobKey);

	SCOPE_LOCK_START(&m_csHandler);
	if (m_pSelfThreadPool)
	{
		m_pSelfThreadPool->Destroy();
		delete m_pSelfThreadPool;
	}
	SCOPE_LOCK_END();

	DeleteCriticalSection(&m_csHandler);
}

BOOL CMessageSender::RegisterHandler(LPMESSAGEHANDLER pHandler, REGPARAM lpParam)
{
	CScopeLock lock(&m_csHandler);

	if (m_mapMessageHandlers.find(pHandler) != m_mapMessageHandlers.end())
		return FALSE;

	m_mapMessageHandlers[pHandler] = lpParam;
	return TRUE;
}

void CMessageSender::UnregisterHandler(LPMESSAGEHANDLER pHandler)
{
	CScopeLock lock(&m_csHandler);

	if (m_mapMessageHandlers.find(pHandler) == m_mapMessageHandlers.end())
		return;

	m_mapMessageHandlers.erase(pHandler);
}


void CMessageSender::NotifyMessageCore(UINT uMessage, MSGPARAM pMsgParam)
{
	std::map<LPMESSAGEHANDLER, REGPARAM> handlers;
	SCOPE_LOCK_START(&m_csHandler);
	handlers = m_mapMessageHandlers;
	SCOPE_LOCK_END();

	std::map<LPMESSAGEHANDLER, REGPARAM>::iterator pos = handlers.begin();
	while(!m_bIsDestroy && (pos != handlers.end()))
	{
		try
		{
			(*pos->first)(uMessage, pMsgParam, pos->second);
		}
		catch(...)
		{ }
		pos++;
	}
}

void CMessageSender::NotifyMessage(UINT uMessage, MSGPARAM pMsgParam, BOOL bAsync/* = FALSE*/)
{
	if (IsLocked(uMessage))
		return;

	if (!bAsync)
	{
		NotifyMessageCore(uMessage, pMsgParam);
	}
	else
	{
		CScopeLock lock(&m_csHandler);
		if (m_bIsDestroy)
			return;

		if (m_pThreadPool)
			m_pThreadPool->AddWorkerFunction(SendWorker, new WORKER_ITEM(this, uMessage, pMsgParam), m_nWorkThreadId, m_nThreadJobKey);
		else
		{
			if (!m_pSelfThreadPool)
			{
				m_pSelfThreadPool = new CSafeThreadPool(TRUE);
				m_pSelfThreadPool->Initialize("DefaultMessageSender", 1);
			}

			if (m_pSelfThreadPool)
				m_pSelfThreadPool->AddWorkerFunction(SendWorker, new WORKER_ITEM(this, uMessage, pMsgParam), 0, m_nThreadJobKey);
		}
	}
}


void CALLBACK CMessageSender::SendWorker(void* arg, size_t key)
{
	WORKER_ITEM* item = (WORKER_ITEM*)arg;
	CMessageSender* pSender = item->pSender;

	pSender->NotifyMessageCore(item->uMessage, item->pMsgParam);

	delete item;
}


void CMessageSender::LockMessage(UINT uMessage)
{
	CScopeLock lock(&m_csHandler);

	if (m_lockMessages.find(uMessage) == m_lockMessages.end())
	{
		m_lockMessages[uMessage] = 1;
	}
	else
	{
		m_lockMessages[uMessage]++;
	}
}


void CMessageSender::UnlockMessage(UINT uMessage)
{
	CScopeLock lock(&m_csHandler);

	if (m_lockMessages.find(uMessage) == m_lockMessages.end())
		return;

	m_lockMessages[uMessage]--;
	if (m_lockMessages[uMessage] <= 0)
	{
		m_lockMessages.erase(uMessage);
	}
}


BOOL CMessageSender::IsLocked(UINT uMessage)
{
	CScopeLock lock(&m_csHandler);

	return (m_lockMessages.find(uMessage) != m_lockMessages.end());
}