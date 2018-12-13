#pragma once

#include "AbstractSensor.h"

class TestSensorA : public AbstractBRSensor
{
public:
	class Data : public SensorData
	{
		friend class TestSensorA;
	public:
		Data() { readNum = 0; }
		int readNum;
	};

	explicit TestSensorA();
	~TestSensorA();

	inline static const char* sName() { return "TestSensorA"; }
	const char* name() const final override;

	void setBaudrate( uint32_t baudrate );
	void setTextMessage( const char* text );
	void setDelay( uint32_t ms );
	void setCpuLoad( uint8_t load );

	Data* readData() final override;
	Data* sensorData() final override;
	uint32_t sensorDataSize() final override;

private:
	uint32_t _baudrate;
	char* _text;
	uint32_t _delay;
	uint8_t _load;
	Data data;
};