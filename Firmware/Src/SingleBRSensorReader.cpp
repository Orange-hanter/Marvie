#include "SingleBRSensorReader.h"
#include "Core/Assert.h"
#include "Core/CriticalSectionLocker.h"

SingleBRSensorReader::SingleBRSensorReader() : BRSensorReader( SINGLE_BR_SENSOR_READER_STACK_SIZE )
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

bool SingleBRSensorReader::startReading( tprio_t prio )
{
	if( tState != State::Stopped || sensor == nullptr )
		return false;

	tState = State::Working;
	setPriority( prio );
	if( !start() )
	{
		tState = State::Stopped;
		return false;
	}
	extEventSource.broadcastFlags( ( eventflags_t )EventFlag::StateChanged );

	return true;
}

void SingleBRSensorReader::stopReading()
{
	CriticalSectionLocker locker;
	if( tState == State::Stopped || tState == State::Stopping )
		return;
	tState = State::Stopping;
	requestInterruption();
	extEventSource.broadcastFlags( ( eventflags_t )EventFlag::StateChanged );
	signalEventsI( InnerEventFlag::StopRequestFlag );
}

void SingleBRSensorReader::forceOne( AbstractBRSensor* forcedSensor )
{
	CriticalSectionLocker locker;
	if( tState == State::Working && forcedSensor == sensor )
		signalEventsI( InnerEventFlag::ForceOneRequestFlag );
}

void SingleBRSensorReader::forceAll()
{
	CriticalSectionLocker locker;
	if( tState == State::Working )
		signalEventsI( InnerEventFlag::ForceOneRequestFlag );
}

AbstractBRSensor* SingleBRSensorReader::nextSensor()
{
	if( tState == State::Stopped )
		return nullptr;
	return sensor;
}

sysinterval_t SingleBRSensorReader::timeToNextReading()
{
	syssts_t sysStatus = chSysGetStatusAndLockX();
	sysinterval_t interval;
	if( tState == BRSensorReader::State::Working && wState == BRSensorReader::WorkingState::Waiting )
	{
		interval = ( sysinterval_t )( nextTime - chVTGetSystemTimeX() );
		if( interval > nextInterval )
			interval = 0;
	}
	else
		interval = 0;
	chSysRestoreStatusX( sysStatus );

	return interval;
}

void SingleBRSensorReader::main()
{
	virtual_timer_t timer;
	chVTObjectInit( &timer );

	chSysLock();
	nextInterval = normalPriod;
	nextTime = ( systime_t )( chVTGetSystemTimeX() + nextInterval );
	chSysUnlock();

	if( nextInterval == 0 )
		signalEvents( InnerEventFlag::TimeoutFlag );
	else
		chVTSet( &timer, nextInterval, timerCallback, this );
	wState = BRSensorReader::WorkingState::Waiting;

	while( tState == State::Working )
	{
		eventmask_t em = chEvtWaitAny( ALL_EVENTS );
		if( em & InnerEventFlag::StopRequestFlag )
			break;
		if( em & InnerEventFlag::ForceOneRequestFlag )
		{
			chVTReset( &timer );
			chEvtGetAndClearEvents( InnerEventFlag::TimeoutFlag );
			em = InnerEventFlag::TimeoutFlag;
		}
		if( em & InnerEventFlag::TimeoutFlag )
		{
			systime_t t0 = chVTGetSystemTimeX();
			wState = BRSensorReader::WorkingState::Reading;
			auto data = sensor->readData();
			sysinterval_t dt = chVTTimeElapsedSinceX( t0 );
			chSysLock();
			wState = BRSensorReader::WorkingState::Waiting;
			nextInterval = data->isValid() ? normalPriod : emergencyPeriod;
			if( nextInterval > dt )
				nextInterval -= dt;
			else
				nextInterval = 0;
			nextTime = ( systime_t )( chVTGetSystemTimeX() + nextInterval );
			chSysUnlock();

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
	wState = WorkingState::Connecting;
	nextInterval = 0;
	extEventSource.broadcastFlags( ( eventflags_t )EventFlag::StateChanged );
	waitingQueue.dequeueNext( MSG_OK );
}

void SingleBRSensorReader::timerCallback( void* p )
{
	chSysLockFromISR();
	reinterpret_cast< SingleBRSensorReader* >( p )->signalEventsI( InnerEventFlag::TimeoutFlag );
	chSysUnlockFromISR();
}