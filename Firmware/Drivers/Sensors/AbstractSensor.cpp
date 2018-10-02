#include "AbstractSensor.h"
#include "Core/ServiceEvent.h"
#include "hal.h"

#define SENSOR_TERMINATE_TEST_INTERVAL TIME_MS2I( 200 )

AbstractSRSensor::AbstractSRSensor()
{
	signalProvider = nullptr;
}

AbstractSensor::Type AbstractSRSensor::type()
{
	return Type::SR;
}

void AbstractSRSensor::setInputSignalProvider( SignalProvider* signalProvider )
{
	this->signalProvider = signalProvider;
}

AbstractSRSensor::SignalProvider* AbstractSRSensor::inputSignalProvider()
{
	return signalProvider;
}

AbstractBRSensor::AbstractBRSensor()
{
	io = nullptr;
}

AbstractBRSensor::Type AbstractBRSensor::type()
{
	return Type::BR;
}

void AbstractBRSensor::setIODevice( IODevice* io )
{
	this->io = io;
}

IODevice* AbstractBRSensor::ioDevice()
{
	return io;
}

ByteRingIterator AbstractBRSensor::waitForResponse( const char* response, uint32_t responseLen, sysinterval_t timeout )
{
	uint32_t pos = 0;
	ByteRingIterator i, respBegin;
	i = respBegin = io->inputBuffer()->begin();

	chEvtGetAndClearEvents( DataServiceEvent | TimeoutServiceEvent );

	virtual_timer_t timer;
	chVTObjectInit( &timer );
	sysinterval_t nextInterval = timeout;
	if( nextInterval > SENSOR_TERMINATE_TEST_INTERVAL )
		nextInterval = SENSOR_TERMINATE_TEST_INTERVAL;
	chVTSet( &timer, nextInterval, timerCallback, chThdGetSelfX() );

	EvtListener ioDeviceListener;
	io->eventSource()->registerMaskWithFlags( &ioDeviceListener, DataServiceEvent, CHN_INPUT_AVAILABLE );
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
		eventmask_t em = chEvtWaitAny( DataServiceEvent | TimeoutServiceEvent );
		if( em & TimeoutServiceEvent )
		{
			timeout -= nextInterval;
			if( timeout == 0 || chThdShouldTerminateX() )
				break;
			if( timeout > SENSOR_TERMINATE_TEST_INTERVAL )
				nextInterval = SENSOR_TERMINATE_TEST_INTERVAL;
			else
				nextInterval = timeout;
			chVTSet( &timer, nextInterval, timerCallback, chThdGetSelfX() );
		}
	}

Leave:
	io->eventSource()->unregister( &ioDeviceListener );
	chVTReset( &timer );

	if( pos == responseLen )
		return respBegin;
	return ByteRingIterator();
}

bool AbstractBRSensor::waitForReadAvailable( uint32_t size, sysinterval_t timeout )
{
	sysinterval_t nextInterval = 0;
	while( timeout )
	{
		if( chThdShouldTerminateX() )
			return false;
		if( timeout > SENSOR_TERMINATE_TEST_INTERVAL )
			nextInterval = SENSOR_TERMINATE_TEST_INTERVAL;
		else
			nextInterval = timeout;
		if( io->waitForReadAvailable( size, nextInterval ) )
			return true;
		timeout -= nextInterval;
	}

	return false;
}

void AbstractBRSensor::timerCallback( void* p )
{
	chSysLockFromISR();
	chEvtSignalI( reinterpret_cast< thread_t* >( p ), TimeoutServiceEvent );
	chSysUnlockFromISR();
}