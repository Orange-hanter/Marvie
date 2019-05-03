#pragma once

#include "Mutex.h"

class MutexLocker
{
    mutex_t* mtx;

public:
    inline MutexLocker( Mutex& mutex ) : mtx ( &mutex.mtx )
    {
        CriticalSectionLocker locker;
        chMtxLockS( mtx );
    }
    inline MutexLocker( DynamicMutex& mutex ) : mtx ( mutex.mtx.get() )
    {
        CriticalSectionLocker locker;
        chMtxLockS( mtx );
    }
    MutexLocker( const Mutex& other ) = delete;
    MutexLocker( Mutex&& other ) = delete;
    inline ~MutexLocker()
    {
        CriticalSectionLocker locker;
        chMtxUnlockS( mtx );
    }
};