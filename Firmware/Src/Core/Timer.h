#include "CriticalSectionLocker.h"
#include "Object.h"
#include <cstring>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

struct AbstractTimerFunctionData
{
	virtual ~AbstractTimerFunctionData() {}
	virtual void call() = 0;
};

template< typename Function, typename... Args >
struct TimerFunctionData : public AbstractTimerFunctionData
{
	TimerFunctionData( Function&& func, Args&&... args ) :
		func( std::forward< Function >( func ) ),
		args( std::forward< Args >( args )... )
	{

	}

	void call() override
	{
		_call( std::index_sequence_for< Args... >{} );
	}

	template< size_t... Values >
	inline void _call( std::index_sequence< Values... > )
	{
		func( std::forward< Args >( std::get< Values >( args ) )... );
	}

	inline void _call( std::index_sequence<> )
	{
		func();
	}

	Function func;
	std::tuple< Args... > args;
};

template< typename Function, typename... Args >
constexpr size_t timerFunctionDataSize = sizeof( TimerFunctionData< Function, Args... > );

class Timer : public Object
{
public:
	Timer();

	template< typename Function, typename... Args, typename = std::enable_if_t< !std::is_same< std::decay_t< Function >, Timer >::value > >
	Timer( Function&& function, Args&&... args )
	{
		uint8_t* block = new uint8_t[sizeof( Impl ) + timerFunctionDataSize< Function, Args... >];
		if( block == nullptr )
			return;
		ptr.reset( reinterpret_cast< Impl* >( block ) );		
		chVTObjectInit( &ptr->vt );
		ptr->intv = 1;
		ptr->single = false;
		ptr->functionData = new( block + sizeof( Impl ) ) TimerFunctionData< Function, Args... >( std::forward< Function >( function ), std::forward< Args >( args )... );
	}

	Timer( const Timer& other ) = delete;
	Timer( Timer&& other );
	~Timer();

	Timer& operator=( const Timer& other ) = delete;
	Timer& operator=( Timer&& other );

	inline bool isValid()
	{
		return ptr != nullptr;
	}

	template< typename Function, typename... Args >
	void callOnTimeout( Function&& function, Args&&... args )
	{
		if( ptr == nullptr )
			return;
		chVTReset( &ptr->vt );
		removeFunctionData();
		ptr->functionData = new TimerFunctionData< Function, Args... >( std::forward< Function >( function ), std::forward< Args >( args )... );
	}

	inline void start( sysinterval_t interval )
	{
		setInterval( interval );
		start();
	}

	inline void start()
	{
		CriticalSectionLocker locker;
		if( ptr == nullptr && ptr->functionData == nullptr )
			return;
		chVTSetI( &ptr->vt, ptr->intv, timerCallback, ptr.get() );
	}

	inline void stop()
	{
		CriticalSectionLocker locker;
		if( ptr == nullptr )
			return;
		chVTResetI( &ptr->vt );
	}

	inline void setInterval( sysinterval_t interval )
	{
		if( ptr )
			ptr->intv = interval;
	}

	inline sysinterval_t interval()
	{
		if( ptr == nullptr )
			return ( sysinterval_t )0;
		return ptr->intv;
	}

	inline void setSingleShot( bool singleShot )
	{
		if( ptr )
			ptr->single = singleShot;
	}

	inline bool isSingleShot()
	{
		if( ptr == nullptr )
			return false;
		return ptr->single;
	}

	template< typename Function, typename... Args >
	static bool singleShot( sysinterval_t interval, Function&& function, Args&&... args )
	{
		struct Block : public Object
		{			
			uint8_t data[sizeof( virtual_timer_t ) + timerFunctionDataSize< Function, Args... >];
			~Block()
			{
				reinterpret_cast< AbstractTimerFunctionData* >( data + sizeof( virtual_timer_t ) )->~AbstractTimerFunctionData();
			}
		}* block = new Block;
		if( !block )
			return false;
		virtual_timer_t* vt = reinterpret_cast< virtual_timer_t* >( block->data );
		chVTObjectInit( vt );
		new( block->data + sizeof( virtual_timer_t ) ) TimerFunctionData< Function, Args... >( std::forward< Function >( function ), std::forward< Args >( args )... );
		chVTSet( vt, interval, timerCallbackWithDeletion, block );

		return true;
	}

	void swap( Timer& other );

private:
	void removeFunctionData();
	static void timerCallback( void* p );
	static void timerCallbackWithDeletion( void* p );

private:
	struct Impl
	{
		virtual_timer_t vt;
		sysinterval_t intv;
		bool single;
		AbstractTimerFunctionData* functionData;
	};
	std::unique_ptr< Impl > ptr;
};

template< size_t FunctionDataSize = 0 >
class StaticTimer : public Object
{
public:
	StaticTimer()
	{
		chVTObjectInit( &vt );
		intv = 1;
		single = false;
		new( block ) TimerFunctionData< void( & )() >( defaultFunction );
	}

	template< typename Function, typename... Args, typename = std::enable_if_t< !std::is_same< std::decay_t< Function >, StaticTimer >::value > >
	StaticTimer( Function&& function, Args&&... args )
	{
		chVTObjectInit( &vt );
		intv = 1;
		single = false;
		new( block ) TimerFunctionData< Function, Args... >( std::forward< Function >( function ), std::forward< Args >( args )... );
	}

	StaticTimer( const StaticTimer& other ) = delete;
	StaticTimer( StaticTimer&& other ) = delete;
	~StaticTimer()
	{
		{
			CriticalSectionLocker locker;
			chVTResetI( &vt );
		}
		removeFunctionData();
	}

	StaticTimer& operator=( const StaticTimer& other ) = delete;
	StaticTimer& operator=( StaticTimer&& other ) = delete;

	template< typename Function, typename... Args >
	void callOnTimeout( Function&& f, Args&&... args )
	{
		static_assert( sizeof( block ) >= timerFunctionDataSize< Function, Args... >, "FunctionDataSize is too small" );
		chVTReset( &vt );
		removeFunctionData();
		new( block ) TimerFunctionData< Function, Args... >( std::forward< Function >( f ), std::forward< Args >( args )... );
	}

	inline void start( sysinterval_t interval )
	{
		setInterval( interval );
		start();
	}

	inline void start()
	{
		CriticalSectionLocker locker;
		chVTSetI( &vt, intv, timerCallback, this );
	}

	inline void stop()
	{
		CriticalSectionLocker locker;
		chVTResetI( &vt );
	}

	inline void setInterval( sysinterval_t interval )
	{
		this->intv = interval;
	}

	inline sysinterval_t interval()
	{
		return intv;
	}

	inline void setSingleShot( bool singleShot )
	{
		this->single = singleShot;
	}

	inline bool isSingleShot()
	{
		return single;
	}

private:
	inline void removeFunctionData()
	{
		reinterpret_cast< AbstractTimerFunctionData* >( block )->~AbstractTimerFunctionData();
	}

	static void timerCallback( void* p )
	{
		StaticTimer* const timer = reinterpret_cast< StaticTimer* >( p );
		if( !timer->single )
		{
			chSysLockFromISR();
			chVTDoSetI( &timer->vt, timer->intv, timerCallback, p );
			chSysUnlockFromISR();
		}
		reinterpret_cast< AbstractTimerFunctionData* >( timer->block )->call();
	}

	static void defaultFunction()
	{

	}

private:
	virtual_timer_t vt;
	sysinterval_t intv;
	bool single;
	enum { MinBlockSize = timerFunctionDataSize< void( & )( void* ), void* > };
	uint8_t block[FunctionDataSize < MinBlockSize ? MinBlockSize : FunctionDataSize];
};

class BasicTimer
{
public:
	inline BasicTimer()
	{
		chVTObjectInit( &vt );
	}
	BasicTimer( const BasicTimer& ) = delete;
	BasicTimer( BasicTimer&& ) = delete;
	inline ~BasicTimer()
	{
		CriticalSectionLocker locker;
		chVTResetI( &vt );
	}

	BasicTimer& operator=( const BasicTimer& other ) = delete;
	BasicTimer& operator=( BasicTimer&& other ) = delete;

	using CallbackFunction = void( * )( void* );
	inline void start( sysinterval_t interval, CallbackFunction function, void* prm = nullptr )
	{
		CriticalSectionLocker locker;
		chVTSetI( &vt, interval, function, prm );
	}

	inline void stop()
	{
		CriticalSectionLocker locker;
		chVTResetI( &vt );
	}

private:
	virtual_timer_t vt;
};