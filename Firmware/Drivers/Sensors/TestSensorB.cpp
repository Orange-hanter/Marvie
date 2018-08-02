#include "TestSensorB.h"
#include "Core/DateTimeService.h"
#include "Drivers/Interfaces/UsartBasedDevice.h"
#include <string.h>
#include <stdio.h>

TestSensorB::TestSensorB()
{
	io = nullptr;
	_text = nullptr;
	goodN = 1;
	badN = 1;
	errType = ErrorType::NoResp;
}

TestSensorB::~TestSensorB()
{
	delete _text;
}

const char* TestSensorB::name() const
{
	return sName();
}

void TestSensorB::setIODevice( IODevice* io )
{
	this->io = io;
}

IODevice* TestSensorB::ioDevice()
{
	return io;
}

void TestSensorB::setBaudrate( uint32_t baudrate )
{
	this->_baudrate = baudrate;
}

void TestSensorB::setTextMessage( const char* text )
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

void TestSensorB::setGoodNum( uint32_t n )
{
	goodN = n;
}

void TestSensorB::setBadNum( uint32_t n )
{
	badN = n;
}

void TestSensorB::setErrorType( ErrorType errType )
{
	this->errType = errType;
}

void TestSensorB::setErrorCode( uint8_t errCode )
{
	data.errCode = errCode;
}

TestSensorB::Data* TestSensorB::readData()
{
	if( !io )
		return &data;

	if( io->isSerialDevice() )
		static_cast< UsartBasedDevice* >( io )->setBaudRate( _baudrate );

	if( _text )
		io->write( ( const uint8_t* )_text, strlen( _text ), TIME_INFINITE );
	else
		io->write( ( const uint8_t* )"Text message", sizeof( "Text message" ) - 1, TIME_INFINITE );
	char str[50];
	bool good = nextIsGood();
	sprintf( str, " : CallNum = %d (%s)\n", ++data.readNum, good ? "good" : "bad" );
	io->write( ( const uint8_t* )str, strlen( str ), TIME_INFINITE );

	data.lock();
	if( good )
		data.errType = SensorData::Error::NoError;
	else
		data.errType = errType == ErrorType::Crc ? SensorData::Error::CrcError : SensorData::Error::NoResponseError;
	data.t = DateTimeService::currentDateTime();
	data.unlock();

	return &data;
}

TestSensorB::Data* TestSensorB::sensorData()
{
	return &data;
}

uint32_t TestSensorB::sensorDataSize()
{
	return sizeof( Data ) - sizeof( SensorData );
}

bool TestSensorB::nextIsGood()
{
	uint32_t g = ( data.readNum + 1 ) % ( goodN + badN );
	return g < goodN;
}