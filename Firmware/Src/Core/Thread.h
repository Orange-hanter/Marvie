#pragma once

#include "AbstractThread.h"
#include "NewDelete.h"
#include "Object.h"
#include <tuple>
#include <type_traits>

template< typename Function, typename... Args >
class _FunctionThread;

struct ThreadProperties
{
	uint32_t stackSize;
	tprio_t priority;
	const char* name;
	MemoryAllocPolicy stackPolicy;

	constexpr inline ThreadProperties() : stackSize( 1024 ), priority( NORMALPRIO ), name( "Untitled" ), stackPolicy( MemoryAllocPolicy::TryCCM ) {}
	constexpr inline ThreadProperties( uint32_t stackSize, tprio_t priority = NORMALPRIO, const char* name = "Untitled", MemoryAllocPolicy stackPolicy = MemoryAllocPolicy::TryCCM )
	    : stackSize( stackSize ), priority( priority ), name( name ), stackPolicy( stackPolicy ) {}
};
constexpr ThreadProperties defaultThreadProperties;

class Thread : public Object, public AbstractThread
{
public:
	Thread() noexcept;
	Thread( uint32_t stackSize, MemoryAllocPolicy stackPolicy = MemoryAllocPolicy::TryCCM ) noexcept;
	Thread( const Thread& other ) = delete;
	Thread( Thread&& other ) noexcept;
	~Thread();

	Thread& operator=( const Thread& other ) = delete;
	Thread& operator=( Thread&& other ) noexcept;

	void setStackSize( uint32_t size, MemoryAllocPolicy memPolicy = MemoryAllocPolicy::TryCCM ) noexcept;
	uint32_t stackSize() noexcept;

	bool start() override;

	template< typename Function, typename... Args, typename = std::enable_if_t< !std::is_same< ThreadProperties, std::decay_t< Function > >::value > >
	static Thread* create( Function&& function, Args&&... args )
	{
		return new _FunctionThread< Function, Args... >( std::forward< Function >( function ), std::forward< Args >( args )... );
	}

	template< typename Function, typename... Args >
	static inline Thread* create( const ThreadProperties& props, Function&& function, Args&&... args )
	{
		auto thread = new _FunctionThread< Function, Args... >( std::forward< Function >( function ), std::forward< Args >( args )... );
		if( thread == nullptr )
			return nullptr;
		thread->setStackSize( props.stackSize, props.stackPolicy );
		thread->setPriority( props.priority );
		thread->setName( props.name );
		return thread;
	}

	template< typename Function, typename... Args, typename = std::enable_if_t< !std::is_same< ThreadProperties, std::decay_t< Function > >::value > >
	static Thread* createAndStart( Function&& function, Args&&... args )
	{
		auto thread = new _FunctionThread< Function, Args... >( std::forward< Function >( function ), std::forward< Args >( args )... );
		if( thread == nullptr )
			return nullptr;
		if( !thread->start() )
		{
			delete thread;
			return nullptr;
		}
		return thread;
	}

	template< typename Function, typename... Args >
	static Thread* createAndStart( const ThreadProperties& props, Function&& function, Args&&... args )
	{
		auto thread = new _FunctionThread< Function, Args... >( std::forward< Function >( function ), std::forward< Args >( args )... );
		if( thread == nullptr )
			return nullptr;
		thread->setStackSize( props.stackSize, props.stackPolicy );
		thread->setPriority( props.priority );
		thread->setName( props.name );
		if( !thread->start() )
		{
			delete thread;
			return nullptr;
		}
		return thread;
	}

	void swap( Thread&& other ) noexcept;

protected:
	void main() override;

private:
	static void thdStart( void* p );

private:
	uint8_t* wa;
	uint32_t size;
	MemoryAllocPolicy stackPolicy;
};

template< typename Function, typename... Args >
class _FunctionThread final : public Thread
{
public:
	_FunctionThread( Function&& _function, Args&&... _args ) : function( std::forward< Function >( _function ) ), args( std::forward< Args >( _args )... )
	{
	}

private:
	Function function;
	std::tuple< Args... > args;

	template< std::size_t... Indices >
	inline void call( std::index_sequence< Indices... > )
	{
		function( std::forward< Args >( std::get< Indices >( args ) )... );
	}

	void main() override
	{
		call( std::index_sequence_for< Args... >{} );
	}
};