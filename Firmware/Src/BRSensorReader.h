#pragma once

#include "Core/Thread.h"
#include "Drivers/Sensors/AbstractSensor.h"
#include "Core/ThreadsQueue.h"

class BRSensorReader : protected Thread
{
public:
	enum EventFlag : eventflags_t { StateChanged = 1, SensorDataUpdated = 2 };
	enum class State { Stopped, Working, Stopping };

	BRSensorReader( uint32_t stackSize );
	virtual ~BRSensorReader();

	virtual bool startReading( tprio_t prio ) = 0;
	virtual void stopReading() = 0;
	State state();
	bool waitForStateChange( sysinterval_t timeout = TIME_INFINITE );

	virtual void forceOne( AbstractBRSensor* ) = 0;
	virtual void forceAll() = 0;

	virtual AbstractBRSensor* nextSensor() = 0;
	virtual sysinterval_t timeToNextReading() = 0;

	EventSourceRef eventSource();

protected:
	bool waitAndCheck( sysinterval_t interval, sysinterval_t step = TIME_MS2I( 100 ) );

protected:
	enum InnerEventFlag : eventflags_t { StopRequestFlag = 1, TimeoutFlag = 2, ForceOneRequestFlag = 4, ForceAllRequestFlag = 8, NetworkFlag = 16 };
	enum class WorkingState { Waiting, Connecting, Reading };

	State tState;
	WorkingState wState;
	EventSource extEventSource;
	ThreadsQueue waitingQueue;
};