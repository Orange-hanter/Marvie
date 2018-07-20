#pragma once

#include "Core/BaseDynamicThread.h"
#include "BRSensorReader.h"

#define SINGLE_BR_SENSOR_READER_STACK_SIZE    512 // 192B min stack size

class SingleBRSensorReader : private BaseDynamicThread, public BRSensorReader
{
public:
	SingleBRSensorReader();
	~SingleBRSensorReader();

	void setSensor( AbstractBRSensor* sensor, sysinterval_t normalPriod, sysinterval_t emergencyPeriod );

	bool startReading( tprio_t prio ) final override;
	void stopReading() final override;

	void forceOne( AbstractBRSensor* ) final override;
	void forceAll() final override;

	AbstractBRSensor* nextSensor() final override;
	sysinterval_t timeToNextReading() final override;

private:
	void main() final override;
	static void timerCallback( void* );

private:
	AbstractBRSensor* sensor;
	sysinterval_t normalPriod;
	sysinterval_t emergencyPeriod;
	sysinterval_t nextInterval;
	systime_t nextTime;
};