#pragma once

class CScopedLock
{
public:
	CScopedLock(CCriticalSection *lock)
		: m_lock(lock)
	{
		m_lock->Lock();
	}

	~CScopedLock()
	{
		m_lock->Unlock();
	}

private:
	CCriticalSection * m_lock;
};