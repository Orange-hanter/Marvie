#pragma once

#include "Drivers/Sensors/AbstractSensor.h"
#include "cpp_wrappers/ch.hpp"

using namespace chibios_rt;

class BRSensorReader
{
public:
	enum EventFlag : eventflags_t { StateChanged = 1, SensorDataUpdated = 2 };
	enum class State { Stopped, Working, Stopping };

	BRSensorReader();
	virtual ~BRSensorReader();

	virtual bool startReading( tprio_t prio ) = 0;
	virtual void stopReading() = 0;
	State state();
	bool waitForStateChange( sysinterval_t timeout = TIME_INFINITE );

	virtual void forceOne( AbstractBRSensor* ) = 0;
	virtual void forceAll() = 0;

	virtual AbstractBRSensor* nextSensor() = 0;
	virtual sysinterval_t timeToNextReading() = 0;

	EvtSource* eventSource();

protected:
	enum InnerEventFlag : eventflags_t { StopRequestFlag = 1, TimeoutFlag = 2, ForceOneRequestFlag = 4, ForceAllRequestFlag = 8 };
	enum class WorkingState { Waiting, Reading };

	State tState;
	WorkingState wState;
	EvtSource extEventSource;
	threads_queue_t waitingQueue;
};