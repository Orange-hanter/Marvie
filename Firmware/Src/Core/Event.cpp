#include "Event.h"
#include "CriticalSectionLocker.h"

EventListener::EventListener()
{
	nativeListener.listener = nullptr;
	eSource = nullptr;
}

EventListener::EventListener( ThreadRef thread )
{
	nativeListener.listener = thread.nativeHandler();
	eSource = nullptr;
}

EventListener::~EventListener()
{
	unregister();
}

void EventListener::setThread( ThreadRef thread )
{
	nativeListener.listener = thread.nativeHandler();
}

ThreadRef EventListener::thread()
{
	return ThreadRef( nativeListener.listener );
}

eventmask_t EventListener::eventMask()
{
	return nativeListener.events;
}

eventflags_t EventListener::getAndClearFlags()
{
	CriticalSectionLocker locker;
	return chEvtGetAndClearFlagsI( &nativeListener );
}

void EventListener::unregister()
{
	CriticalSectionLocker locker;
	if( eSource )
	{
		chEvtUnregisterI( reinterpret_cast< event_source_t* >( eSource ), &nativeListener );
		eSource = nullptr;
	}
}

bool EventListener::isListening()
{
	return eSource != nullptr;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EventSource::EventSource()
{
	chEvtObjectInit( &nativeSource );
}

EventSource::~EventSource()
{
	unregisterAll();
}

EventSourceRef EventSource::reference()
{
	return EventSourceRef( this );
}

void EventSource::registerMaskWithFlags( EventListener* listener, eventmask_t emask, eventflags_t eflags )
{
	CriticalSectionLocker locker;
	if( listener->eSource == this )
	{
		listener->nativeListener.events = emask;
		listener->nativeListener.wflags = eflags;
		return;
	}
	else if( listener->eSource != nullptr )
		listener->unregister();
	if( listener->nativeListener.listener == nullptr )
		listener->nativeListener.listener = currp;
	chEvtRegisterThreadMaskWithFlagsI( &nativeSource, &listener->nativeListener, listener->nativeListener.listener, emask, eflags );
	listener->eSource = this;
}

void EventSource::unregister( EventListener* listener )
{
	CriticalSectionLocker locker;
	if( listener->eSource != this )
		return;
	chEvtUnregisterI( &nativeSource, &listener->nativeListener );
	listener->eSource = nullptr;
}

void EventSource::unregisterAll()
{
	CriticalSectionLocker locker;
	event_listener_t* elp = nativeSource.next;
	while( elp != reinterpret_cast< event_listener_t * >( &nativeSource ) )
	{
		reinterpret_cast< EventListener* >( elp )->eSource = nullptr;
		elp = elp->next;
	}
	chEvtObjectInit( &nativeSource );
}

uint32_t EventSource::registeredListenerCount()
{
	CriticalSectionLocker locker;
	uint32_t n = 0;
	event_listener_t* elp = nativeSource.next;
	while( elp != reinterpret_cast< event_listener_t * >( &nativeSource ) )
	{
		++n;
		elp = elp->next;
	}

	return n;
}

void EventSource::broadcastFlags( eventflags_t flags )
{
	CriticalSectionLocker locker;
	chEvtBroadcastFlagsI( &nativeSource, flags );
}