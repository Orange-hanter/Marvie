#pragma once

#include "BRSensorReader.h"

#define SINGLE_BR_SENSOR_READER_STACK_SIZE    1024 // 192B min stack size

class SingleBRSensorReader : public BRSensorReader
{
public:
	SingleBRSensorReader();
	~SingleBRSensorReader();

	void setSensor( AbstractBRSensor* sensor, sysinterval_t normalPriod, sysinterval_t emergencyPeriod );

	bool startReading( tprio_t prio ) override;
	void stopReading() override;

	void forceOne( AbstractBRSensor* ) override;
	void forceAll() override;

	AbstractBRSensor* nextSensor() override;
	sysinterval_t timeToNextReading() override;

protected:
	void main() override;
	static void timerCallback( void* );

protected:
	AbstractBRSensor* sensor;
	sysinterval_t normalPriod;
	sysinterval_t emergencyPeriod;
	sysinterval_t nextInterval;
	systime_t nextTime;
};