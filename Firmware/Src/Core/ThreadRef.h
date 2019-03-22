#pragma once

#include "ch.h"
#include <cstddef>

class ThreadRef final
{
	thread_t* threadRef;

public:
	inline ThreadRef() : threadRef( nullptr ) {}
	inline ThreadRef( thread_t* tp ) : threadRef( tp ) {}
	ThreadRef( const ThreadRef& other ) = default;
	ThreadRef( ThreadRef&& other ) = default;

	ThreadRef& operator=( const ThreadRef& other ) = default;
	ThreadRef& operator=( ThreadRef&& other ) = default;

	inline bool operator==( const ThreadRef& other )
	{
		return threadRef == other.threadRef;
	}

	inline bool operator!=( const ThreadRef& other )
	{
		return threadRef != other.threadRef;
	}

	inline bool operator==( std::nullptr_t )
	{
		return threadRef == nullptr;
	}

	static inline ThreadRef currentThread()
	{
		return ThreadRef( currp );
	}

	inline bool isNull() const
	{
		return threadRef == nullptr;
	}

	inline thread_t* nativeHandler()
	{
		return threadRef;
	}

	inline void requestInterruption() const
	{
		chThdTerminate( threadRef );
	}

#if( CH_CFG_USE_REGISTRY == TRUE ) || defined( __DOXYGEN__ )
	inline ThreadRef addRef() const
	{
		return ThreadRef( chThdAddRef( threadRef ) );
	}

	inline void release()
	{
		thread_t* tp = threadRef;
		threadRef = nullptr;

		chThdRelease( tp );
	}
#endif /* CH_CFG_USE_REGISTRY == TRUE */

#if( CH_CFG_USE_WAITEXIT == TRUE ) || defined( __DOXYGEN__ )
	inline msg_t wait()
	{
		thread_t* tp = threadRef;
		threadRef = nullptr;

		msg_t msg = chThdWait( tp );
		return msg;
	}
#endif /* CH_CFG_USE_WAITEXIT == TRUE */

#if( CH_CFG_USE_MESSAGES == TRUE ) || defined( __DOXYGEN__ )
	inline msg_t sendMessage( msg_t msg ) const
	{
		return chMsgSend( threadRef, msg );
	}

	inline bool isPendingMessage() const
	{
		return chMsgIsPendingI( threadRef );
	}

	inline msg_t message() const
	{
		return chMsgGet( threadRef );
	}

	inline void releaseMessage( msg_t msg )
	{
		thread_t* tp = threadRef;
		threadRef = nullptr;

		chMsgRelease( tp, msg );
	}
#endif /* CH_CFG_USE_MESSAGES == TRUE */

#if( CH_CFG_USE_EVENTS == TRUE ) || defined( __DOXYGEN__ )
	inline void signalEvents( eventmask_t mask ) const
	{
		chEvtSignal( threadRef, mask );
	}

	inline void signalEventsI( eventmask_t mask ) const
	{
		chEvtSignalI( threadRef, mask );
	}
#endif /* CH_CFG_USE_EVENTS == TRUE */

#if( CH_DBG_THREADS_PROFILING == TRUE ) || defined( __DOXYGEN__ )
	inline systime_t ticksX() const
	{
		return chThdGetTicksX( threadRef );
	}
#endif /* CH_DBG_THREADS_PROFILING == TRUE */
};