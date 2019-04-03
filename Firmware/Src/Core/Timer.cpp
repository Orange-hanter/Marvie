#include "Timer.h"

Timer::Timer() : ptr( new Impl )
{
	if( ptr )
	{
		chVTObjectInit( &ptr->vt );
		ptr->intv = 1;
		ptr->single = false;
		ptr->functionData = nullptr;
	}
}

Timer::Timer( Timer&& other )
{
	swap( other );
}

Timer::~Timer()
{
	if( ptr == nullptr )
		return;
	chVTReset( &ptr->vt );
	removeFunctionData();
}

void Timer::swap( Timer& other )
{
	ptr.swap( other.ptr );
}

void Timer::removeFunctionData()
{
	if( reinterpret_cast< uint8_t* >( &ptr->vt ) + sizeof( Impl ) == reinterpret_cast< uint8_t* >( ptr->functionData ) )
	{
		ptr->functionData->~AbstractTimerFunctionData();
		return;
	}
	delete ptr->functionData;
}

void Timer::timerCallback( void* p )
{
	Impl* const impl = reinterpret_cast< Impl* >( p );
	if( !impl->single )
	{
		chSysLockFromISR();
		chVTDoSetI( &impl->vt, impl->intv, timerCallback, p );
		chSysUnlockFromISR();
	}
	impl->functionData->call();
}

void Timer::timerCallbackWithDeletion( void* p )
{
	Object* obj = reinterpret_cast< Object* >( p );
	reinterpret_cast< AbstractTimerFunctionData* >( ( uint8_t* )p + sizeof( Object ) + sizeof( virtual_timer_t ) )->call();
	obj->deleteLater();
}

Timer& Timer::operator=( Timer&& other )
{
	swap( other );
	return *this;
}
