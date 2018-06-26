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

	virtual void startReading( tprio_t prio ) = 0;
	virtual void stopReading() = 0;
	State state();
	bool waitForStateChange( sysinterval_t timeout = TIME_INFINITE );

	EvtSource* eventSource();

protected:
	enum InnerEventFlag : eventflags_t { StopRequestFlag = 1, TimeoutFlag = 2 };

	State tState;
	EvtSource extEventSource;
	threads_queue_t waitingQueue;
};