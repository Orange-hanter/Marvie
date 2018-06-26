#include "BRSensorReader.h"

BRSensorReader::BRSensorReader()
{
	tState = State::Stopped;
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