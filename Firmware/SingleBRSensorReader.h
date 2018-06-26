#pragma once

#include "BRSensorReader.h"
#include "cpp_wrappers/ch.hpp"

class SingleBRSensorReader : private BaseStaticThread< 192 >, public BRSensorReader // 192b min stack size
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