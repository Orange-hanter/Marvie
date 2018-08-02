#include "BRSensorReader.h"

BRSensorReader::BRSensorReader()
{
	tState = State::Stopped;
	wState = WorkingState::Connecting;
	chThdQueueObjectInit( &waitingQueue );
}

BRSensorReader::~BRSensorReader()
{

}

BRSensorReader::State BRSensorReader::state()
{
	return tState;
}

bool BRSensorReader::waitForStateChange( sysinterval_t timeout /*= TIME_INFINITE */ )
{
	msg_t msg = MSG_OK;
	chSysLock();
	if( tState == State::Stopping )
		msg = chThdEnqueueTimeoutS( &waitingQueue, timeout );
	chSysUnlock();

	return msg == MSG_OK;
}

EvtSource* BRSensorReader::eventSource()
{
	return &extEventSource;
}

bool BRSensorReader::waitAndCheck( sysinterval_t interval, sysinterval_t step /*= TIME_MS2I( 100 ) */ )
{
	for( uint32_t i = 0; i < interval / step; ++i )
	{
		if( tState != BRSensorReader::State::Working )
			return false;
		chThdSleep( step );
	}

	return true;
}