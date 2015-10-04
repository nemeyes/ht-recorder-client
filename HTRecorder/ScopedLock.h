#pragma once

class CScopedLock
{
public:
	CScopedLock( CRITICAL_SECTION *lock );
	~CScopedLock( VOID );

private:
	CScopedLock( CScopedLock& clone );

	CRITICAL_SECTION *m_lock;
};