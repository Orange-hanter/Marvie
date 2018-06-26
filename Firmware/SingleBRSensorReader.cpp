#include "SingleBRSensorReader.h"
#include "Core/Assert.h"

SingleBRSensorReader::SingleBRSensorReader() : BRSensorReader()
{
	sensor = nullptr;
	normalPriod = emergencyPeriod = 0;
}

SingleBRSensorReader::~SingleBRSensorReader()
{
	stopReading();
	waitForStateChange();
}

void SingleBRSensorReader::setSensor( AbstractBRSensor* sensor, sysinterval_t normalPriod, sysinterval_t emergencyPeriod )
{
	this->sensor = sensor;
	this->normalPriod = normalPriod;
	this->emergencyPeriod = emergencyPeriod;
}

void SingleBRSensorReader::startReading( tprio_t prio )
{
	if( tState != State::Stopped || sensor == nullptr )
	{
		assert( false );
		return;
	}

	tState = State::Working;
	extEventSource.broadcastFlags( ( eventflags_t )EventFlag::StateChanged );
	start( prio );
}

void SingleBRSensorReader::stopReading()
{

	chSysLock();
	if( tState == State::Stopped || tState == State::Stopping )
	{
		chSysUnlock();
		return;
	}
	tState = State::Stopping;
	extEventSource.broadcastFlagsI( ( eventflags_t )EventFlag::StateChanged );
	signalEventsI( InnerEventFlag::StopRequestFlag );
	chSchRescheduleS();
	chSysUnlock();
}

void SingleBRSensorReader::main()
{
	getAndClearEvents( ALL_EVENTS );

	virtual_timer_t timer;
	chVTObjectInit( &timer );
	if( normalPriod == 0 )
		signalEvents( InnerEventFlag::TimeoutFlag );
	else
		chVTSet( &timer, normalPriod, timerCallback, this );

	while( tState == State::Working )
	{
		eventmask_t em = chEvtWaitAny( ALL_EVENTS );
		if( em & InnerEventFlag::StopRequestFlag )
			break;
		if( em & InnerEventFlag::TimeoutFlag )
		{
			systime_t t0 = chVTGetSystemTimeX();
			auto data = sensor->readData();
			sysinterval_t dt = chVTTimeElapsedSinceX( t0 );
			sysinterval_t nextInterval = data->isValid() ? normalPriod : emergencyPeriod;
			if( nextInterval > dt )
				nextInterval -= dt;
			else
				nextInterval = 0;
			if( nextInterval == 0 )
				signalEvents( InnerEventFlag::TimeoutFlag );
			else
				chVTSet( &timer, nextInterval, timerCallback, this );
			extEventSource.broadcastFlags( EventFlag::SensorDataUpdated );
		}
	}

	chVTReset( &timer );

	chSysLock();
	tState = State::Stopped;
	extEventSource.broadcastFlagsI( ( eventflags_t )EventFlag::StateChanged );
	chThdDequeueNextI( &waitingQueue, MSG_OK );
	exitS( MSG_OK );
}

void SingleBRSensorReader::timerCallback( void* p )
{
	chSysLockFromISR();
	reinterpret_cast< SingleBRSensorReader* >( p )->signalEventsI( InnerEventFlag::TimeoutFlag );
	chSysUnlockFromISR();
}