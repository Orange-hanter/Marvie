#include "NetworkSensorReader.h"

NetworkSensorReader::NetworkSensorReader()
{
	port = 0;
}

NetworkSensorReader::~NetworkSensorReader()
{

}

void NetworkSensorReader::setSensorAddress( IpAddress addr, uint16_t port )
{
	if( tState != State::Stopped )
		return;
	this->addr = addr;
	this->port = port;
}

bool NetworkSensorReader::startReading( tprio_t prio )
{
	if( port == 0 )
		return false;
	return SingleBRSensorReader::startReading( prio );
}

void NetworkSensorReader::main()
{
	virtual_timer_t timer;
	chVTObjectInit( &timer );
	EventListener socketListener;
	socket.eventSource().registerMask( &socketListener, InnerEventFlag::NetworkFlag );
	signalEvents( InnerEventFlag::NetworkFlag );

	sensor->setIODevice( &socket );

	chSysLock();
	nextInterval = normalPriod;
	nextTime = ( systime_t )( chVTGetSystemTimeX() + nextInterval );
	chSysUnlock();

	while( tState == State::Working )
	{
		eventmask_t em = chEvtWaitAny( ALL_EVENTS );
		if( em & InnerEventFlag::StopRequestFlag )
			break;
		if( em & InnerEventFlag::NetworkFlag )
		{
			if( !socket.isOpen() )
			{
				sysinterval_t nextInterval = timeToNextReading();
				wState = BRSensorReader::WorkingState::Connecting;
				chVTReset( &timer );
				chEvtGetAndClearEvents( InnerEventFlag::TimeoutFlag );
				while( !socket.isOpen() )
				{
					systime_t t0 = chVTGetSystemTimeX();
					if( !socket.connect( addr, port ) )
					{
						if( !waitAndCheck( TIME_MS2I( 3000 ) ) )
							goto End;
					}
					else
					{
						if( !waitAndCheck( TIME_MS2I( 1000 ) ) )
							goto End;
					}
					sysinterval_t dt = chVTTimeElapsedSinceX( t0 );
					if( nextInterval > dt )
						nextInterval -= dt;
					else
						nextInterval = 0;
				}

				if( nextInterval == 0 )
					em |= InnerEventFlag::TimeoutFlag;
				else
					chVTSet( &timer, nextInterval, timerCallback, this );
				nextTime = ( systime_t )( chVTGetSystemTimeX() + nextInterval );
				wState = BRSensorReader::WorkingState::Waiting;
			}
		}
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

End:
	socketListener.unregister();
	chVTReset( &timer );
	socket.disconnect();

	chSysLock();
	tState = State::Stopped;
	wState = WorkingState::Connecting;
	nextInterval = 0;
	extEventSource.broadcastFlags( ( eventflags_t )EventFlag::StateChanged );
	waitingQueue.dequeueNext( MSG_OK );
}