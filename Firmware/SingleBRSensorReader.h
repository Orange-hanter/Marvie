#pragma once

#include "Core/BaseDynamicThread.h"
#include "BRSensorReader.h"

#define SINGLE_BR_SENSOR_READER_STACK_SIZE    192 // 192B min stack size

class SingleBRSensorReader : private BaseDynamicThread, public BRSensorReader
{
public:
	SingleBRSensorReader();
	~SingleBRSensorReader();

	void setSensor( AbstractBRSensor* sensor, sysinterval_t normalPriod, sysinterval_t emergencyPeriod );

	void startReading( tprio_t prio ) final override;
	void stopReading() final override;

private:
	void main() final override;
	static void timerCallback( void* );

private:
	AbstractBRSensor* sensor;
	sysinterval_t normalPriod;
	sysinterval_t emergencyPeriod;
};