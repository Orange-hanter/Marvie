#pragma once

#include "ThreadRef.h"
#include <cstddef>

class EventSource;
class EventSourceRef;

class EventListener
{
public:
	EventListener();
	EventListener( ThreadRef thread );
	EventListener( const EventListener& other ) = delete;
	EventListener( EventListener&& other ) = delete;
	~EventListener();

	EventListener& operator=( const EventListener& other ) = delete;
	EventListener& operator=( EventListener&& other ) = delete;

	void setThread( ThreadRef thread );
	eventflags_t getAndClearFlags();
	void unregister();
	bool isListening();

private:
	friend class EventSource;
	friend class EventSourceRef;
	event_listener_t nativeListener;
	EventSource* eSource;
};

class EventSource
{
public:
	EventSource();
	EventSource( const EventSource& other ) = delete;
	EventSource( EventSource&& other ) = delete;
	~EventSource();

	EventSource& operator=( const EventSource& other ) = delete;
	EventSource& operator=( EventSource&& other ) = delete;

	EventSourceRef reference();

	inline void registerOne( EventListener* listener, eventid_t eid )
	{
		registerMaskWithFlags( listener, EVENT_MASK( eid ), ( eventflags_t )-1 );
	}
	inline void registerMask( EventListener* listener, eventmask_t emask )
	{
		registerMaskWithFlags( listener, emask, ( eventflags_t )-1 );
	}
	void registerMaskWithFlags( EventListener* listener, eventmask_t emask, eventflags_t eflags );
	void unregister( EventListener* listener );
	void unregisterAll();
	uint32_t registeredListenerCount();

	void broadcastFlags( eventflags_t flags );

private:
	event_source_t nativeSource;
};

class EventSourceRef
{
public:
	inline EventSourceRef() : eSource( nullptr ){};
	inline EventSourceRef( EventSource* source ) : eSource( source ){};
	inline EventSourceRef( event_source_t* source ) : eSource( reinterpret_cast< EventSource* >( source ) ){};
	EventSourceRef( const EventSourceRef& other ) = default;
	EventSourceRef( EventSourceRef&& other ) = default;

	EventSourceRef& operator=( const EventSourceRef& other ) = default;
	EventSourceRef& operator=( EventSourceRef&& other ) = default;

	inline bool operator==( const EventSourceRef& other )
	{
		return eSource == other.eSource;
	}
	inline bool operator!=( const EventSourceRef& other )
	{
		return eSource != other.eSource;
	}
	inline bool operator==( std::nullptr_t )
	{
		return eSource == nullptr;
	}

	inline bool isNull()
	{
		return eSource == nullptr;
	}

	inline void registerOne( EventListener* listener, eventid_t eid )
	{
		eSource->registerMaskWithFlags( listener, EVENT_MASK( eid ), ( eventflags_t )-1 );
	}
	inline void registerMask( EventListener* listener, eventmask_t emask )
	{
		eSource->registerMaskWithFlags( listener, emask, ( eventflags_t )-1 );
	}
	inline void registerMaskWithFlags( EventListener* listener, eventmask_t emask, eventflags_t eflags )
	{
		eSource->registerMaskWithFlags( listener, emask, eflags );
	}
	inline void unregister( EventListener* listener )
	{
		eSource->unregister( listener );
	}

private:
	EventSource* eSource;
};