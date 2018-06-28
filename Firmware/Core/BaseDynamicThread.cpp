#include "BaseDynamicThread.h"
#include "CCMemoryHeap.h"

BaseDynamicThread::BaseDynamicThread( uint32_t stackSize, bool tryCCM /*= true*/ )
{
	waSize = THD_WORKING_AREA_SIZE( stackSize );
	if( tryCCM )
	{
		wa = reinterpret_cast< uint8_t* >( CCMemoryHeap::alloc( waSize ) );
		if( wa == nullptr )
			wa = reinterpret_cast< uint8_t* >( chHeapAlloc( nullptr, waSize ) );
	}
	else
		wa = reinterpret_cast< uint8_t* >( chHeapAlloc( nullptr, waSize ) );
}

BaseDynamicThread::~BaseDynamicThread()
{
	delete wa;
}

ThreadReference BaseDynamicThread::start( tprio_t prio )
{
	thread_ref = chThdCreateStatic( wa, waSize, prio, _thd_start, this );
	return *this;
}

void BaseDynamicThread::_thd_start( void* arg )
{
	static_cast< BaseDynamicThread* >( arg )->main();
}