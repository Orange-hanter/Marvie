#pragma once

#include "ThreadRef.h"
#include <utility>

class AbstractThread
{
protected:
	thread_t* threadRef;
	const char* threadName;
	tprio_t threadPrio;

public:
	AbstractThread() noexcept : threadRef( nullptr ), threadName( nullptr ), threadPrio( NORMALPRIO ) {};
	AbstractThread( const AbstractThread& other ) = delete;
	AbstractThread( AbstractThread&& other ) noexcept : AbstractThread()
	{
		swap( std::move( other ) );
	}
	virtual ~AbstractThread() {}

	AbstractThread& operator=( const AbstractThread& other ) = delete;
	AbstractThread& operator=( AbstractThread&& other ) noexcept
	{
		swap( std::move( other ) );
		return *this;
	}

	virtual bool start() = 0;
	inline bool start( tprio_t prio )
	{
		setPriority( prio );
		return start();
	}

	inline void setName( const char* name )
	{
		threadName = name;
#if CH_CFG_USE_REGISTRY == TRUE
		if( threadRef )
			threadRef->name = name;
#endif
	}

	inline tprio_t setPriority( tprio_t newprio )
	{
		threadPrio = newprio;
		if( threadRef )
			newprio = chThdSetPriority( newprio );
		return newprio;
	}

	inline tprio_t priority()
	{
		return threadPrio;
	}

	inline void swap( AbstractThread&& other ) noexcept
	{
		std::swap( threadRef, other.threadRef );
		std::swap( threadName, other.threadName );
		std::swap( threadPrio, other.threadPrio );
	}

protected:
	virtual void main() = 0;

public:
	inline ThreadRef threadReference()
	{
		return ThreadRef( threadRef );
	}

	inline thread_t* nativeHandle()
	{
		return threadRef;
	}

	inline void requestInterruption()
	{
		chThdTerminate( threadRef );
	}

	inline bool isInterruptionRequested() const
	{
		return ( bool )( ( threadRef->flags & CH_FLAG_TERMINATE ) != ( tmode_t )0 );
	}

	inline msg_t wait()
	{
		return chThdWait( threadRef );
	}

	inline msg_t sendMessage( msg_t msg )
	{
		return chMsgSend( threadRef, msg );
	}

	inline bool isPendingMessage() const
	{
		return chMsgIsPendingI( const_cast< thread_t* >( threadRef ) );
	}

	inline msg_t message()
	{
		return chMsgGet( threadRef );
	}

	inline void releaseMessage( msg_t msg )
	{
		chMsgRelease( threadRef, msg );
	}

	inline void signalEvents( eventmask_t mask )
	{
		chEvtSignal( threadRef, mask );
	}

	inline void signalEventsI( eventmask_t mask )
	{
		chEvtSignalI( threadRef, mask );
	}

	static inline void exit( msg_t msg )
	{
		chThdExit( msg );
	}

	static inline void exitS( msg_t msg )
	{
		chThdExitS( msg );
	}

	static inline void sleep( sysinterval_t interval )
	{
		chThdSleep( interval );
	}

	static inline void sleepUntil( systime_t time )
	{
		chThdSleepUntil( time );
	}

	static inline systime_t sleepUntilWindowed( systime_t prev, systime_t next )
	{
		return chThdSleepUntilWindowed( prev, next );
	}

	static inline void yield()
	{
		chThdYield();
	}

	static inline ThreadRef waitMessage()
	{
		return ThreadRef( chMsgWait() );
	}

	static inline eventmask_t getAndClearEvents( eventmask_t mask )
	{
		return chEvtGetAndClearEvents( mask );
	}

	static inline eventmask_t addEvents( eventmask_t mask )
	{
		return chEvtAddEvents( mask );
	}

	static inline eventmask_t waitOneEvent( eventmask_t ewmask )
	{
		return chEvtWaitOne( ewmask );
	}

	static inline eventmask_t waitAnyEvent( eventmask_t ewmask )
	{
		return chEvtWaitAny( ewmask );
	}

	static inline eventmask_t waitAllEvents( eventmask_t ewmask )
	{
		return chEvtWaitAll( ewmask );
	}

	static inline eventmask_t waitOneEventTimeout( eventmask_t ewmask, sysinterval_t timeout )
	{
		return chEvtWaitOneTimeout( ewmask, timeout );
	}

	static inline eventmask_t waitAnyEventTimeout( eventmask_t ewmask, sysinterval_t timeout )
	{
		return chEvtWaitAnyTimeout( ewmask, timeout );
	}

	static inline eventmask_t waitAllEventsTimeout( eventmask_t ewmask, sysinterval_t timeout )
	{
		return chEvtWaitAllTimeout( ewmask, timeout );
	}

	static inline void unlockAllMutexes()
	{
		chMtxUnlockAll();
	}
};