#include "TestSRSensorA.h"
#include "Core/DateTimeService.h"

TestSRSensorA::TestSRSensorA()
{
	blockId = 0;
	line = 0;
}

TestSRSensorA::~TestSRSensorA()
{

}

const char* TestSRSensorA::name() const
{
	return sName();
}

void TestSRSensorA::setSignalChannel( uint16_t blockId, uint16_t line )
{
	this->blockId = blockId;
	this->line = line;
}

TestSRSensorA::Data* TestSRSensorA::readData()
{
	data.t = DateTimeService::currentDateTime();
	data.value = signalProvider->analogSignal( blockId, line );

	return &data;
}

TestSRSensorA::Data* TestSRSensorA::sensorData()
{
	return &data;
}

uint32_t TestSRSensorA::sensorDataSize()
{
	return sizeof( Data ) - sizeof( SensorData );
}