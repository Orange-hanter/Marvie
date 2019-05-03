#pragma once

#include "Assert.h"
#include "CriticalSectionLocker.h"
#include <memory>

class BinarySemaphore
{
public:
	BinarySemaphore( bool taken = true ) noexcept
	{
		chBSemObjectInit( &sem, taken );
	}
	BinarySemaphore( const BinarySemaphore& other ) = delete;
	BinarySemaphore( BinarySemaphore&& other ) = delete;
	~BinarySemaphore() noexcept
	{
		if( sem.sem.cnt < 0 )
			chSysHalt( "destroying a semaphore that is in use" );
	}

	BinarySemaphore& operator=( const BinarySemaphore& other ) = delete;
	BinarySemaphore& operator=( BinarySemaphore&& other ) = delete;

	inline bool acquire() noexcept
	{
		CriticalSectionLocker locker;
		return chBSemWaitS( &sem ) == MSG_OK;
	}
	inline bool acquire( sysinterval_t timeout ) noexcept
	{
		CriticalSectionLocker locker;
		return chBSemWaitTimeoutS( &sem, timeout ) == MSG_OK;
	}
	inline void release() noexcept
	{
		CriticalSectionLocker locker;
		chBSemSignalI( &sem );
	}
	inline void reset( bool taken = true ) noexcept
	{
		CriticalSectionLocker locker;
		chBSemResetI( &sem, taken );
	}
	inline bool isTaken() noexcept
	{
		CriticalSectionLocker locker;
		return chBSemGetStateI( &sem );
	}

private:
	binary_semaphore_t sem;
};

class Semaphore
{
public:
	Semaphore( cnt_t n = 1 ) noexcept
	{
		chSemObjectInit( &sem, n );
	}
	Semaphore( const Semaphore& other ) = delete;
	Semaphore( Semaphore&& other ) = delete;
    ~Semaphore() noexcept
	{
		if( sem.cnt < 0 )
			chSysHalt( "destroying a semaphore that is in use" );
	}

	Semaphore& operator=( const Semaphore& other ) = delete;
	Semaphore& operator=( Semaphore&& other ) = delete;

	inline bool acquire() noexcept
	{
		CriticalSectionLocker locker;
		return chSemWaitS( &sem ) == MSG_OK;
	}
	inline bool acquire( sysinterval_t timeout = TIME_INFINITE ) noexcept
	{
		CriticalSectionLocker locker;
		return chSemWaitTimeoutS( &sem, timeout ) == MSG_OK;
	}
	inline void release() noexcept
	{
		CriticalSectionLocker locker;
		chSemSignalI( &sem );
	}
	inline void release( cnt_t n ) noexcept
	{
		CriticalSectionLocker locker;
		chSemAddCounterI( &sem, n );
	}
	inline void reset( cnt_t n = 1 ) noexcept
	{
		CriticalSectionLocker locker;
		chSemResetI( &sem, n );
	}
	inline int available() noexcept
	{
		CriticalSectionLocker locker;
		return chSemGetCounterI( &sem );
	}

private:
	semaphore_t sem;
};

class DynamicBinarySemaphore
{
public:
	DynamicBinarySemaphore( bool taken = true ) noexcept : sem( new binary_semaphore_t )
	{
		chBSemObjectInit( sem.get(), taken );
	}
	DynamicBinarySemaphore( const DynamicBinarySemaphore& other ) = delete;
	DynamicBinarySemaphore( DynamicBinarySemaphore&& other ) noexcept
	{
		std::swap( sem, other.sem );
	}
    ~DynamicBinarySemaphore() noexcept
	{
		if( sem->sem.cnt < 0 )
			chSysHalt( "destroying a semaphore that is in use" );
	}

	DynamicBinarySemaphore& operator=( const DynamicBinarySemaphore& other ) = delete;
	DynamicBinarySemaphore& operator=( DynamicBinarySemaphore&& other ) noexcept
	{
		std::swap( sem, other.sem );
		return *this;
	}

	inline bool acquire() noexcept
	{
		CriticalSectionLocker locker;
		return chBSemWaitS( sem.get() ) == MSG_OK;
	}
	inline bool acquire( sysinterval_t timeout ) noexcept
	{
		CriticalSectionLocker locker;
		return chBSemWaitTimeoutS( sem.get(), timeout ) == MSG_OK;
	}
	inline void release() noexcept
	{
		CriticalSectionLocker locker;
		chBSemSignalI( sem.get() );
	}
	inline void reset( bool taken = true ) noexcept
	{
		CriticalSectionLocker locker;
		chBSemResetI( sem.get(), taken );
	}
	inline bool isTaken() noexcept
	{
		CriticalSectionLocker locker;
		return chBSemGetStateI( sem.get() );
	}

private:
	std::unique_ptr< binary_semaphore_t > sem;
};

class DynamicSemaphore
{
public:
	DynamicSemaphore( cnt_t n = 1 ) noexcept : sem( new semaphore_t )
	{
		chSemObjectInit( sem.get(), n );
	}
	DynamicSemaphore( const DynamicSemaphore& other ) = delete;
	DynamicSemaphore( DynamicSemaphore&& other ) noexcept
	{
		std::swap( sem, other.sem );
	}
    ~DynamicSemaphore() noexcept
	{
		if( sem->cnt < 0 )
			chSysHalt( "destroying a semaphore that is in use" );
	}

	DynamicSemaphore& operator=( const DynamicSemaphore& other ) = delete;
	DynamicSemaphore& operator=( DynamicSemaphore&& other ) noexcept
	{
		std::swap( sem, other.sem );
		return *this;
	}

	inline bool acquire() noexcept
	{
		CriticalSectionLocker locker;
		return chSemWaitS( sem.get() ) == MSG_OK;
	}
	inline bool acquire( sysinterval_t timeout = TIME_INFINITE ) noexcept
	{
		CriticalSectionLocker locker;
		return chSemWaitTimeoutS( sem.get(), timeout ) == MSG_OK;
	}
	inline void release() noexcept
	{
		CriticalSectionLocker locker;
		chSemSignalI( sem.get() );
	}
	inline void release( cnt_t n ) noexcept
	{
		CriticalSectionLocker locker;
		chSemAddCounterI( sem.get(), n );
	}
	inline void reset( cnt_t n = 1 ) noexcept
	{
		CriticalSectionLocker locker;
		chSemResetI( sem.get(), n );
	}
	inline int available() noexcept
	{
		CriticalSectionLocker locker;
		return chSemGetCounterI( sem.get() );
	}

private:
	std::unique_ptr< semaphore_t > sem;
};