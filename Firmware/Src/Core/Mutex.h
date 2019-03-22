#pragma once

#include "CriticalSectionLocker.h"
#include <memory>

class Mutex
{
public:
	Mutex() noexcept
	{
		chMtxObjectInit( &mtx );
	}
	Mutex( const Mutex& other ) = delete;
	Mutex( Mutex&& other ) = delete;
	~Mutex()
	{
		if( mtx.owner )
			chSysHalt( "attempt to delete a owned mutex" );
	}

	Mutex& operator=( const Mutex& other ) = delete;
	Mutex& operator=( Mutex&& other ) = delete;

	inline void lock() noexcept
	{
		CriticalSectionLocker locker;
		chMtxLockS( &mtx );
	}
	inline void unlock() noexcept
	{
		CriticalSectionLocker locker;
		chMtxUnlockS( &mtx );
	}
	inline void tryLock() noexcept
	{
		CriticalSectionLocker locker;
		chMtxTryLockS( &mtx );
	}

private:
	mutex_t mtx;
};

class DynamicMutex
{
public:
	DynamicMutex() : mtx( new mutex_t )
	{
		chMtxObjectInit( mtx.get() );
	}
	DynamicMutex( const DynamicMutex& other ) = delete;
	DynamicMutex( DynamicMutex&& other )
	{
		std::swap( mtx, other.mtx );
	}
	~DynamicMutex()
	{
		if( mtx != nullptr && mtx->owner )
			chSysHalt( "attempt to delete a owned mutex" );
	}

	DynamicMutex& operator=( const DynamicMutex& other ) = delete;
	DynamicMutex& operator=( DynamicMutex&& other )
	{
		std::swap( mtx, other.mtx );
		return *this;
	}

	inline void lock() noexcept
	{
		CriticalSectionLocker locker;
		chMtxLockS( mtx.get() );
	}
	inline void unlock() noexcept
	{
		CriticalSectionLocker locker;
		chMtxUnlockS( mtx.get() );
	}
	inline void tryLock() noexcept
	{
		CriticalSectionLocker locker;
		chMtxTryLockS( mtx.get() );
	}

private:
	std::unique_ptr< mutex_t > mtx;
};