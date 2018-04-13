#pragma once

#include "ch.h"
#include "RingIterator.h"
#include "ReverseRingIterator.h"

template< typename Type >
class BaseRingBuffer
{
public:
	typedef RingIterator< Type > Iterator;
	typedef ReverseRingIterator< Type > ReverseIterator;

	BaseRingBuffer( Type* buffer, uint32_t size );
	~BaseRingBuffer();

	uint32_t write( const Type* data, uint32_t size, sysinterval_t timeout );
	uint32_t write( const Type* data, uint32_t size );
	uint32_t writeAvailable() const;

	uint32_t read( Type* data, uint32_t size, sysinterval_t timeout );
	uint32_t read( Type* data, uint32_t size );
	uint32_t readAvailable() const;
	bool waitForReadAvailable( uint32_t size, sysinterval_t timeout );
	bool isOverflowed() const;
	void resetOverflowFlag();
	void clear();

	Type& first();
	Type& back();
	Type& peek( uint32_t index );

	Iterator begin();
	Iterator end();
	ReverseIterator rbegin();
	ReverseIterator rend();

private:
	void copy( Type* dst, const Type* src, uint32_t size );
	uint32_t _read( Type* data, uint32_t size );
	void _write( const Type* data, uint32_t size );

private:
	threads_queue_t readWaitingQueue, writeWaitingQueue;
	Type* buffer;
	volatile uint32_t size;
	volatile uint32_t counter;
	Type* top;
	Type* wrPtr;
	Type* rdPtr;
	volatile bool overflowFlag;
};

template< typename Type, uint32_t Size >
class StaticRingBuffer : public BaseRingBuffer< Type >
{
public:
	StaticRingBuffer() : BaseRingBuffer< Type >( staticBuffer, Size ) {}

private:
	Type staticBuffer[Size + 1];
};

template< typename Type >
class DynamicRingBuffer : public BaseRingBuffer< Type >
{
public:
	DynamicRingBuffer( uint32_t size ) : BaseRingBuffer< Type >( dynamicBuffer = new Type[size + 1], size ) {}
	~DynamicRingBuffer() { delete dynamicBuffer; }

private:
	Type* dynamicBuffer;
};

#if CH_CFG_INTERVALS_SIZE != CH_CFG_TIME_TYPES_SIZE
#error "CH_CFG_INTERVALS_SIZE != CH_CFG_TIME_TYPES_SIZE"
#endif

template< typename Type >
BaseRingBuffer< Type >::BaseRingBuffer( Type* buffer, uint32_t size )
{
	chThdQueueObjectInit( &readWaitingQueue );
	chThdQueueObjectInit( &writeWaitingQueue );
	this->buffer = buffer;
	this->size = size;
	counter = 0;
	top = buffer + size + 1;
	wrPtr = rdPtr = buffer;
	overflowFlag = false;
}

template< typename Type >
BaseRingBuffer< Type >::~BaseRingBuffer()
{
	clear();
}

template< typename Type >
uint32_t BaseRingBuffer< Type >::write( const Type* data, uint32_t size, sysinterval_t timeout )
{
	systime_t deadline = chVTGetSystemTimeX() + ( systime_t )timeout;
	uint32_t available;
	msg_t msg;

	syssts_t sysStatus = chSysGetStatusAndLockX();
	while( true )
	{
		available = this->size - counter;
		if( available < size )
		{
			if( timeout == TIME_INFINITE || timeout == TIME_IMMEDIATE )
				msg = chThdEnqueueTimeoutS( &writeWaitingQueue, timeout );
			else
			{
				sysinterval_t nextTimeout = ( sysinterval_t )( deadline - chVTGetSystemTimeX() );

				// timeout expired
				if( nextTimeout > timeout )
					break;

				msg = chThdEnqueueTimeoutS( &writeWaitingQueue, nextTimeout );
			}

			if( msg == MSG_RESET )
			{
				chSysRestoreStatusX( sysStatus );
				return 0;
			}
			else if( msg != MSG_OK )
			{
				available = this->size - counter;
				break;
			}
		}
		else
		{
			_write( data, size );
			chSysRestoreStatusX( sysStatus );
			return size;
		}
	}

	_write( data, available );
	overflowFlag = true;
	chSysRestoreStatusX( sysStatus );
	return available;
}

template< typename Type >
uint32_t BaseRingBuffer< Type >::write( const Type* data, uint32_t size )
{
	syssts_t sysStatus = chSysGetStatusAndLockX();
	uint32_t available = this->size - counter;
	if( available < size )
	{
		size = available;
		overflowFlag = true;
	}
	_write( data, size );
	chSysRestoreStatusX( sysStatus );
	return size;
}

template< typename Type >
uint32_t BaseRingBuffer< Type >::writeAvailable() const
{
	return size - counter;
}

template< typename Type >
uint32_t BaseRingBuffer< Type >::read( Type* data, uint32_t size, sysinterval_t timeout )
{
	chDbgCheck( size > 0U );

	uint32_t n = size;
	systime_t deadline = chVTGetSystemTimeX() + ( systime_t )timeout;

	syssts_t sysStatus = chSysGetStatusAndLockX();
	while( n )
	{
		uint32_t done = _read( data, n );
		if( done == 0 )
		{
			msg_t msg;
			if( timeout == TIME_INFINITE || timeout == TIME_IMMEDIATE )
				msg = chThdEnqueueTimeoutS( &readWaitingQueue, timeout );
			else
			{
				sysinterval_t nextTimeout = ( sysinterval_t )( deadline - chVTGetSystemTimeX() );

				// timeout expired
				if( nextTimeout > timeout )
					break;

				msg = chThdEnqueueTimeoutS( &readWaitingQueue, nextTimeout );
			}

			// timeout expired or queue reseted
			if( msg != MSG_OK )
				break;
		}
		else
		{
			if( data != nullptr )
				data += done;
			n -= done;
		}
	}

	chSysRestoreStatusX( sysStatus );
	return size - n;
}

template< typename Type >
uint32_t BaseRingBuffer< Type >::read( Type* data, uint32_t size )
{
	syssts_t sysStatus = chSysGetStatusAndLockX();
	uint32_t done = _read( data, size );
	chSysRestoreStatusX( sysStatus );
	return done;
}

template< typename Type >
uint32_t BaseRingBuffer< Type >::readAvailable() const
{
	return counter;
}

template< typename Type >
bool BaseRingBuffer< Type >::waitForReadAvailable( uint32_t size, sysinterval_t timeout )
{
	chDbgCheck( size > 0U );

	systime_t deadline = chVTGetSystemTimeX() + ( systime_t )timeout;

	syssts_t sysStatus = chSysGetStatusAndLockX();
	while( size > counter )
	{
		msg_t msg;
		if( timeout == TIME_INFINITE || timeout == TIME_IMMEDIATE )
			msg = chThdEnqueueTimeoutS( &readWaitingQueue, timeout );
		else
		{
			sysinterval_t nextTimeout = ( sysinterval_t )( deadline - chVTGetSystemTimeX() );

			// timeout expired
			if( nextTimeout > timeout )
				break;

			msg = chThdEnqueueTimeoutS( &readWaitingQueue, nextTimeout );
		}

		// timeout expired or queue reseted
		if( msg != MSG_OK )
			break;
	}

	chSysRestoreStatusX( sysStatus );
	return size <= counter;
}

template< typename Type >
bool BaseRingBuffer< Type >::isOverflowed() const
{
	return overflowFlag;
}

template< typename Type >
void BaseRingBuffer< Type >::resetOverflowFlag()
{
	overflowFlag = false;
}

template< typename Type >
void BaseRingBuffer< Type >::clear()
{
	syssts_t sysStatus = chSysGetStatusAndLockX();
	chThdDequeueAllI( &writeWaitingQueue, MSG_RESET );
	chThdDequeueNextI( &readWaitingQueue, MSG_RESET );
	counter = 0;
	wrPtr = rdPtr = buffer;
	overflowFlag = false;
	chSysRestoreStatusX( sysStatus );
}

template< typename Type >
Type& BaseRingBuffer< Type >::first()
{
	return *rdPtr;
}

template< typename Type >
Type& BaseRingBuffer< Type >::back()
{
	if( wrPtr == buffer )
		return buffer[size];
	return *( wrPtr - 1 );
}

template< typename Type >
Type& BaseRingBuffer< Type >::peek( uint32_t index )
{
	if( index >= size )
		index %= size;
	Type* p = rdPtr + index;
	if( p >= top )
		p -= size + 1;
	return *p;
}

template< typename Type >
typename BaseRingBuffer< Type >::Iterator BaseRingBuffer< Type >::begin()
{
	return Iterator( buffer, rdPtr, size );
}

template< typename Type >
typename BaseRingBuffer< Type >::Iterator BaseRingBuffer< Type >::end()
{
	return Iterator( buffer, wrPtr, size );
}

template< typename Type >
typename BaseRingBuffer< Type >::ReverseIterator BaseRingBuffer< Type >::rbegin()
{
	return ++ReverseIterator( buffer, wrPtr, size );
}

template< typename Type >
typename BaseRingBuffer< Type >::ReverseIterator BaseRingBuffer< Type >::rend()
{
	return ++ReverseIterator( buffer, rdPtr, size );
}

template< typename Type >
void BaseRingBuffer< Type >::copy( Type* dst, const Type* src, uint32_t size )
{
	while( size-- )
		*dst++ = *src++;
}

template< typename Type >
uint32_t BaseRingBuffer< Type >::_read( Type* data, uint32_t size )
{
	if( size > counter )
		size = counter;

	if( data != nullptr )
	{
		uint32_t s1 = top - rdPtr;
		if( size < s1 )
		{
			copy( data, rdPtr, size );
			rdPtr += size;
		}
		else if( size > s1 )
		{
			copy( data, rdPtr, s1 );
			data += s1;
			uint32_t s2 = size - s1;
			copy( data, buffer, s2 );
			rdPtr = buffer + s2;
		}
		else // size == s1
		{
			copy( data, rdPtr, size );
			rdPtr = buffer;
		}
	}
	else
	{
		rdPtr += size;
		if( rdPtr >= top )
			rdPtr = rdPtr - top + buffer;
	}

	counter -= size;
	chThdDequeueAllI( &writeWaitingQueue, MSG_OK );
	chSchRescheduleS();
	return size;
}

template< typename Type >
void BaseRingBuffer< Type >::_write( const Type* data, uint32_t size )
{
	uint32_t s1 = top - wrPtr;
	if( size < s1 )
	{
		copy( wrPtr, data, size );
		wrPtr += size;
	}
	else if( size > s1 )
	{
		copy( wrPtr, data, s1 );
		data += s1;
		uint32_t s2 = size - s1;
		copy( buffer, data, s2 );
		wrPtr = buffer + s2;
	}
	else // size == s1
	{
		copy( wrPtr, data, size );
		wrPtr = buffer;
	}

	counter += size;
	chThdDequeueNextI( &readWaitingQueue, MSG_OK );
	chSchRescheduleS();
}