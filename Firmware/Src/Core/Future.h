#pragma once

#include "Assert.h"
#include "CriticalSectionLocker.h"
#include "Event.h"
#include "Thread.h"
#include "ThreadsQueue.h"
#include <memory>

template< typename Function, typename Type, typename... Args >
class _FutureThread;

template< typename = void >
struct _BaseFutureData
{
	enum State
	{
		WaitingResult,
		ResultAvailable
	} state;
	ThreadsQueue waitingQueue;
	EventSource eventSource;
	virtual ~_BaseFutureData() {}

	void setValue()
	{
		CriticalSectionLocker locker;
		state = State::ResultAvailable;
		waitingQueue.dequeueAll( MSG_OK );
		eventSource.broadcastFlags( ( eventflags_t )1 );
	}

	bool wait( sysinterval_t timeout )
	{
		CriticalSectionLocker locker;
		if( state == State::WaitingResult )
			return waitingQueue.enqueueSelf( timeout ) == MSG_OK;
		return true;
	}
};

template< typename Type >
struct _FutureData : public _BaseFutureData<>
{
	Type value;

	void setValue( const Type& value )
	{
		this->value = value;
		_BaseFutureData<>::setValue();
	}
	void setValue( Type&& value )
	{
		this->value = std::move( value );
		_BaseFutureData<>::setValue();
	}
};

template< typename Type >
struct _FutureData< Type& > : public _BaseFutureData<>
{
	Type* value;

	void setValue( Type& value )
	{
		this->value = reinterpret_cast< Type* >( &value );
		_BaseFutureData<>::setValue();
	}
};

template<>
struct _FutureData< void > : public _BaseFutureData<>
{
};

template< typename Type >
class Future
{
	using SharedFutureData = std::shared_ptr< _FutureData< Type > >;
	SharedFutureData sharedData;

	template< typename Function, typename... Args >
	friend auto _createFutureThread( const ThreadProperties& props, Function&& function, Args&&... args ) -> Future< decltype( function( args... ) ) >;
	friend class Future< void >;

	Future( const SharedFutureData& sharedData ) : sharedData( sharedData ) {}

public:
	Future()
	{
	}

	Future( const Future& other ) : sharedData( other.sharedData )
	{
	}

	Future( Future&& other ) : sharedData( std::move( other.sharedData ) )
	{
	}

	~Future()
	{
	}

	Future& operator=( const Future& other )
	{
		sharedData = other.sharedData;
		return *this;
	}

	Future& operator=( Future&& other )
	{
		sharedData = std::move( other.sharedData );
		return *this;
	}

	bool isValid()
	{
		return sharedData != nullptr;
	}

	bool wait( sysinterval_t timeout = TIME_INFINITE )
	{
		if( !sharedData )
			return false;
		return sharedData->wait( timeout );
	}

	EventSourceRef eventSource()
	{
		if( sharedData )
			return sharedData->eventSource.reference();
		return EventSourceRef();
	}

	Type get()
	{
		assert( sharedData != nullptr );
		sharedData->wait( TIME_INFINITE );
		return static_cast< std::conditional_t< std::is_copy_constructible< Type >::value, const Type&, Type&& > >( sharedData->value );
	}
};

template< typename Type >
class Future< Type& >
{
	using SharedFutureData = std::shared_ptr< _FutureData< Type& > >;
	SharedFutureData sharedData;

	template< typename Function, typename... Args >
	friend auto _createFutureThread( const ThreadProperties& props, Function&& function, Args&&... args ) -> Future< decltype( function( args... ) ) >;
	friend class Future< void >;

	Future( const SharedFutureData& sharedData ) : sharedData( sharedData ) {}

public:
	Future()
	{
	}

	Future( const Future& other ) : sharedData( other.sharedData )
	{
	}

	Future( Future&& other ) : sharedData( std::move( other.sharedData ) )
	{
	}

	~Future()
	{
	}

	Future& operator=( const Future& other )
	{
		sharedData = other.sharedData;
		return *this;
	}

	Future& operator=( Future&& other )
	{
		sharedData = std::move( other.sharedData );
		return *this;
	}

	bool isValid()
	{
		return sharedData != nullptr;
	}

	bool wait( sysinterval_t timeout = TIME_INFINITE )
	{
		if( !sharedData )
			return false;
		return sharedData->wait( timeout );
	}

	EventSourceRef eventSource()
	{
		if( sharedData )
			return sharedData->eventSource.reference();
		return EventSourceRef();
	}

	Type& get()
	{
		assert( sharedData != nullptr );
		sharedData->wait( TIME_INFINITE );
		return *sharedData->value;
	}
};

template<>
class Future< void >
{
	using SharedFutureData = std::shared_ptr< _BaseFutureData<> >;
	SharedFutureData sharedData;

	template< typename Function, typename... Args >
	friend auto _createFutureThread( const ThreadProperties& props, Function&& function, Args&&... args ) -> Future< decltype( function( args... ) ) >;

	Future( const SharedFutureData& sharedData ) : sharedData( sharedData )
	{
	}

	Future( SharedFutureData&& sharedData ) : sharedData( std::move( sharedData ) )
	{
	}

	template< typename Type >
	Future( const std::shared_ptr< _FutureData< Type > >& sharedData ) : sharedData( sharedData )
	{
	}

	template< typename Type >
	Future( std::shared_ptr< _FutureData< Type > >&& sharedData ) : sharedData( std::move( sharedData ) )
	{
	}

public:
	Future()
	{
	}

	template< typename Type >
	Future( const Future< Type >& other ) : Future( other.sharedData )
	{
	}

	template< typename Type >
	Future( Future< Type >&& other ) : Future( std::move( other.sharedData ) )
	{
	}

	~Future()
	{
	}

	template< typename Type >
	Future& operator=( const Future< Type >& other )
	{
		sharedData = other.sharedData;
		return *this;
	}

	template< typename Type >
	Future& operator=( Future< Type >&& other )
	{
		sharedData = std::move( other.sharedData );
		return *this;
	}

	bool isValid()
	{
		return sharedData != nullptr;
	}

	bool wait( sysinterval_t timeout = TIME_INFINITE )
	{
		if( !sharedData )
			return false;
		return sharedData->wait( timeout );
	}

	EventSourceRef eventSource()
	{
		if( sharedData )
			return sharedData->eventSource.reference();
		return EventSourceRef();
	}

	void get()
	{
		assert( sharedData != nullptr );
		sharedData->wait( TIME_INFINITE );
		return;
	}
};

template< typename Function, typename Type, typename... Args >
class _FutureThread : public Thread
{
public:
	using FunctionResultType = decltype( std::declval< Function >()( std::declval< Args >()... ) );
	using SharedFutureData = std::shared_ptr< _FutureData< FunctionResultType > >;
	_FutureThread( SharedFutureData& sharedData, Function&& _function, Args&&... _args ) : sharedData( sharedData ), function( std::forward< Function >( _function ) ), args( std::forward< Args >( _args )... )
	{
	}

	SharedFutureData sharedData;
	Function function;
	std::tuple< Args... > args;

	template< std::size_t... Indices >
	inline void call( std::index_sequence< Indices... > )
	{
		sharedData->setValue( function( std::get< Indices >( args )... ) );
	}

	void main() override
	{
		call( std::index_sequence_for< Args... >{} );
		chSysLock();
		deleteLater();
	}
};

template< typename Function, typename... Args >
class _FutureThread< Function, void, Args... > : public Thread
{
public:
	using FunctionResultType = decltype( std::declval< Function >()( std::declval< Args >()... ) );
	using SharedFutureData = std::shared_ptr< _FutureData< FunctionResultType > >;
	_FutureThread( SharedFutureData& sharedData, Function&& _function, Args&&... _args ) : sharedData( sharedData ), function( std::forward< Function >( _function ) ), args( std::forward< Args >( _args )... )
	{
	}

	SharedFutureData sharedData;
	Function function;
	std::tuple< Args... > args;

	template< std::size_t... Indices >
	inline void call( std::index_sequence< Indices... > )
	{
		function( std::get< Indices >( args )... );
		sharedData->setValue();
	}

	void main() override
	{
		call( std::index_sequence_for< Args... >{} );
		chSysLock();
		deleteLater();
	}
};

template< typename Function, typename... Args >
auto _createFutureThread( const ThreadProperties& props, Function&& function, Args&&... args ) -> Future< decltype( function( args... ) ) >
{
	using FunctionResultType = decltype( function( args... ) );
	auto sharedFutureData = std::make_shared< _FutureData< FunctionResultType > >();
	auto thread = new _FutureThread< Function, FunctionResultType, Args... >( sharedFutureData, std::forward< Function >( function ), std::forward< Args >( args )... );
	if( thread == nullptr )
		return Future< FunctionResultType >();
	thread->setStackSize( props.stackSize, props.stackPolicy );
	thread->setPriority( props.priority );
	thread->setName( props.name );
	if( !thread->start() )
	{
		delete thread;
		return Future< FunctionResultType >();
	}

	return Future< FunctionResultType >( sharedFutureData );
}