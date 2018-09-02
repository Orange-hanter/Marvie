#pragma once

#include "AbstractSensor.h"

class TestSRSensorA : public AbstractSRSensor
{
public:
	class Data : public SensorData
	{
		friend class TestSRSensorA;
	public:
		Data() { value = 0.0f; }
		float value;
	};

	explicit TestSRSensorA();
	~TestSRSensorA();

	inline static const char* sName() { return "TestSRSensorA"; }
	const char* name() const final override;

	void setSignalChannel( uint16_t blockId, uint16_t line );

	Data* readData() final override;
	Data* sensorData() final override;
	uint32_t sensorDataSize() final override;

private:
	uint16_t blockId;
	uint16_t line;
	Data data;
};