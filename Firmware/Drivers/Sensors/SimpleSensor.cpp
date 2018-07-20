#include "SimpleSensor.h"
#include "Core/DateTimeService.h"
#include <string.h>
#include <stdio.h>

SimpleSensor::SimpleSensor()
{
	io = nullptr;
	address = 0;
	data.errType = SensorData::Error::NoError;
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
	data.errType = SensorData::Error::NoError;
	++data.readNum;
	data.t = DateTimeService::currentDateTime();
	data.unlock();
	if( io )
	{
		char str[40];
		sprintf( str, "Addr = %d, ReadNum = %d\n", address, data.readNum );
		io->write( ( const uint8_t* )str, strlen( str ), TIME_INFINITE );

		/*uint32_t load = 55;
		for( int i = 0; i < 500 / 10; ++i )
		{
			auto t0 = chVTGetSystemTime();
			auto t1 = t0 + TIME_MS2I( 10 * load / 100 );
			while( chVTIsSystemTimeWithin( t0, t1 ) );
			chThdSleep( TIME_MS2I( 10 * ( 100 - load ) / 100 ) );
		}*/
	}

	return &data;
}

SimpleSensor::Data* SimpleSensor::sensorData()
{
	return &data;
}

uint32_t SimpleSensor::sensorDataSize()
{
	return sizeof( Data ) - sizeof( SensorData );
}
