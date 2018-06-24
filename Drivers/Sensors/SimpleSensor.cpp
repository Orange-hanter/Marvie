#include "SimpleSensor.h"
#include "Core/DataTimeService.h"
#include <string.h>
#include <stdio.h>

SimpleSensor::SimpleSensor()
{
	io = nullptr;
	address = 0;
	data.valid = true;
}

SimpleSensor::~SimpleSensor()
{

}

const char* SimpleSensor::name() const
{
	return sName();
}

void SimpleSensor::setAddress( uint8_t address )
{
	this->address = address;
}

void SimpleSensor::setIODevice( IODevice* io )
{
	this->io = io;
}

IODevice* SimpleSensor::ioDevice()
{
	return io;
}

SimpleSensor::Data* SimpleSensor::readData()
{
	chThdSleepMilliseconds( 10 );
	data.lock();
	++data.readNum;
	data.t = DataTimeService::currentDataTime();
	data.unlock();
	if( io )
	{
		char str[40];
		sprintf( str, "Addr = %d, ReadNum = %d\n", address, data.readNum );
		io->write( ( const uint8_t* )str, strlen( str ), TIME_INFINITE );
	}

	return &data;
}

SimpleSensor::Data* SimpleSensor::sensorData()
{
	return &data;
}