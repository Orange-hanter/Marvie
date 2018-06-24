#pragma once

#include "AbstractSensor.h"

class SimpleSensor : public AbstractBRSensor
{
public:
	class Data : public SensorData
	{
		friend class SimpleSensor;
	public:
		Data() { readNum = 0; }
		int readNum;
	};

	explicit SimpleSensor();
	virtual ~SimpleSensor();

	inline static const char* sName() { return "SimpleSensor"; }
	const char* name() const final override;
	void setAddress( uint8_t address );	
	void setIODevice( IODevice* ) final override;
	IODevice* ioDevice() final override;

	Data* readData() final override;
	Data* sensorData() final override;

private:
	IODevice * io;
	uint8_t address;
	Data data;
};