#include "stdafx.h"
#include "Recorder/ScopedLock.h"

#ifdef WITH_HITRON_RECORDER

CScopedLock::CScopedLock( CRITICAL_SECTION *lock )
	: m_lock(lock)
{
	::EnterCriticalSection( m_lock );
}

CScopedLock::~CScopedLock( VOID )
{
	::LeaveCriticalSection( m_lock );
}

#endif