#include "AbstractSensor.h"
#include "hal.h"

ByteRingIterator AbstractBRSensor::waitForResponse( IODevice* io, const char* response, uint32_t responseLen, sysinterval_t timeout )
{
	uint32_t pos = 0;
	ByteRingIterator i, respBegin;
	i = respBegin = io->inputBuffer()->begin();

	enum Event : eventmask_t { IODeviceEvent = EVENT_MASK( 0 ), InnerEvent = EVENT_MASK( 1 ) };
	eventmask_t prevEmask = chEvtGetAndClearEvents( IODeviceEvent | InnerEvent );

	virtual_timer_t timer;
	chVTObjectInit( &timer );
	chVTSet( &timer, timeout, timerCallback, chThdGetSelfX() );

	EvtListener ioDeviceListener;
	io->eventSource()->registerMaskWithFlags( &ioDeviceListener, IODeviceEvent, CHN_INPUT_AVAILABLE );
	while( true )
	{
		ByteRingIterator end = io->inputBuffer()->end();
		while( i != end )
		{
			if( *i == response[pos++] )
			{
				if( pos == responseLen )
					goto Leave;
				++i;
			}
			else
			{
				pos = 0;
				i = ++respBegin;
			}
		}
		eventmask_t em = chEvtWaitAny( IODeviceEvent | InnerEvent );
		if( em & InnerEvent )
			break;
		if( em & IODeviceEvent )
			ioDeviceListener.getAndClearFlags();
	}

Leave:
	io->eventSource()->unregister( &ioDeviceListener );
	chVTReset( &timer );

	chEvtAddEvents( prevEmask );
	if( pos == responseLen )
		return respBegin;
	return ByteRingIterator();
}

void AbstractBRSensor::timerCallback( void* p )
{
	chSysLockFromISR();
	chEvtSignalI( reinterpret_cast< thread_t* >( p ), EVENT_MASK( 1 ) ); // InnerEvent
	chSysUnlockFromISR();
}