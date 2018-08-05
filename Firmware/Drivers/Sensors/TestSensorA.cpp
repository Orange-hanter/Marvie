#include "Core/DateTimeService.h"
#include "Drivers/Interfaces/UsartBasedDevice.h"
#include "TestSensorA.h"
#include <string.h>
#include <stdio.h>

TestSensorA::TestSensorA()
{
	_text = nullptr;
}

TestSensorA::~TestSensorA()
{
	delete _text;
}

const char* TestSensorA::name() const
{
	return sName();
}

void TestSensorA::setBaudrate( uint32_t baudrate )
{
	_baudrate = baudrate;
}

void TestSensorA::setTextMessage( const char* text )
{
	delete _text;
	_text = nullptr;
	size_t len = strlen( text );
	if( !len )
		return;
	_text = new char[len + 1];
	strncpy( _text, text, len );
	_text[len] = 0;
}

void TestSensorA::setDelay( uint32_t ms )
{
	_delay = ms;
}

void TestSensorA::setCpuLoad( uint8_t load )
{
	if( load > 100 )
		load = 100;
	_load = load;
}

TestSensorA::Data* TestSensorA::readData()
{
	if( !io )
		return &data;

	if( io->isSerialDevice() )
		static_cast< UsartBasedDevice* >( io )->setBaudRate( _baudrate );

	if( _text )
		io->write( ( const uint8_t* )_text, strlen( _text ), TIME_INFINITE );
	else
		io->write( ( const uint8_t* )"Text message", sizeof( "Text message" ) - 1, TIME_INFINITE );
	char str[40];
	sprintf( str, " : CallNum = %d\n", data.readNum );
	io->write( ( const uint8_t* )str, strlen( str ), TIME_INFINITE );

	for( uint i = 0; i < _delay / 10; ++i )
	{
		auto t0 = chVTGetSystemTime();
		auto t1 = t0 + TIME_US2I( 10000 * _load / 100 );
		while( chVTIsSystemTimeWithin( t0, t1 ) );
		sysinterval_t interval = TIME_US2I( 10000 * ( 100 - _load ) / 100 );
		if( interval )
			chThdSleep( interval );
	}

	data.lock();
	data.errType = SensorData::Error::NoError;
	++data.readNum;
	data.t = DateTimeService::currentDateTime();
	data.unlock();

	return &data;
}

TestSensorA::Data* TestSensorA::sensorData()
{
	return &data;
}

uint32_t TestSensorA::sensorDataSize()
{
	return sizeof( Data ) - sizeof( SensorData );
}
