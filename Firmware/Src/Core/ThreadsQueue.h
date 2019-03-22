#pragma once

#include "CriticalSectionLocker.h"

class ThreadsQueue
{
	threads_queue_t threads_queue;

public:
	inline ThreadsQueue() noexcept
	{
		chThdQueueObjectInit( &threads_queue );
	}
	ThreadsQueue( const ThreadsQueue& other ) = delete;
	ThreadsQueue( ThreadsQueue&& other ) = delete;
	inline ~ThreadsQueue() noexcept
	{
		dequeueAll( MSG_RESET );
	}

	ThreadsQueue& operator=( const ThreadsQueue& ) = delete;
	ThreadsQueue& operator=( ThreadsQueue&& ) = delete;

	inline msg_t enqueueSelf( sysinterval_t timeout ) noexcept
	{
		CriticalSectionLocker locker;
		return chThdEnqueueTimeoutS( &threads_queue, timeout );
	}

	inline void dequeueNext( msg_t msg ) noexcept
	{
		CriticalSectionLocker locker;
		chThdDequeueNextI( &threads_queue, msg );
	}

	inline void dequeueAll( msg_t msg ) noexcept
	{
		CriticalSectionLocker locker;
		chThdDequeueAllI( &threads_queue, msg );
	}
};