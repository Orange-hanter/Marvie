#include "Art01Sensor.h"
#include "Core/DateTimeService.h"

const char Art01Sensor::Name[] = "ART01";

Art01Sensor::Art01Sensor()
{
	address = 0;
	baudrate = 9600;
}

Art01Sensor::~Art01Sensor()
{

}

const char* Art01Sensor::name() const
{
	return Name;
}

void Art01Sensor::setAddress( uint8_t address )
{
	this->address = address;
}

void Art01Sensor::setBaudrate( uint32_t baudrate )
{
	this->baudrate = baudrate;
}

Art01Sensor::Data* Art01Sensor::readData()
{
	io->acquireDevice();
#define returnIfError( err, code ) \
{ \
	if( err != SensorData::Error::NoError ) \
	{ \
		data.lock(); \
		data.errType = err; \
		data.errCode = code; \
		data.unlock(); \
		io->releaseDevice(); \
		io->read( nullptr, io->readAvailable() ); \
		return &data; \
	} \
}

	if( io->isSerialDevice() )
	{
		UsartBasedDevice* serialDevice = static_cast< UsartBasedDevice* >( io );
		serialDevice->setDataFormat( UsartBasedDevice::B8N );
		serialDevice->setStopBits( UsartBasedDevice::S1 );
		serialDevice->setBaudRate( baudrate );
	}
	io->read( nullptr, io->readAvailable() );

	// read instant values
	//                               STRT  ADDR     CMD    /-------------------Emty-data---------------------/  CS
	uint8_t readTimerRequest[13] = { 0x00, address, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42 };
	readTimerRequest[13] = checksum( readTimerRequest, readTimerRequest + 13 );
	io->write( readTimerRequest, sizeof( readTimerRequest ) );
	auto err = waitResponse( 13 );
	returnIfError( err, 1 );
	parseTimerResponse();

	data.lock();
	data.t = DateTimeService::currentDateTime();
	data.errType = SensorData::Error::NoError;
	data.unlock();
	io->read( nullptr, io->readAvailable() );

	io->releaseDevice();
	return &data;
}

Art01Sensor::Data* Art01Sensor::sensorData()
{
	return &data;
}

uint32_t Art01Sensor::sensorDataSize()
{
	return sizeof( Data ) - sizeof( SensorData );
}

SensorData::Error Art01Sensor::waitResponse( uint32_t size )
{
	if( !waitForReadAvailable( size + 1, TIME_MS2I( 2000 ) ) )
		return SensorData::Error::NoResponseError;
	uint8_t addr, cmd;
	io->peek( 1, nullptr, 1 );
	io->peek( 2, &addr, 1 );
    io->peek( 3, &cmd, 1 );
	if( addr != address || cmd != 0x53 )
		return SensorData::Error::CrcError;
	uint8_t cs = checksum( io->inputBuffer()->begin(), io->inputBuffer()->end() - 1 );
	if( cs != *( io->inputBuffer()->end() - 1 ) )
		return SensorData::Error::CrcError;

	return SensorData::Error::NoError;
}

constexpr inline uint32_t reverse( uint32_t data )
{
	return ( ( data & 0x000000FF ) << 24 ) | ( ( data & 0x0000FF00 ) << 8 ) | ( ( data & 0x00FF0000 ) >> 8 ) | ( ( data & 0xFF000000 ) >> 24 );
}

constexpr inline uint16_t reverse( uint16_t data )
{
	return ( ( data & 0x00FF ) << 8 ) | ( ( data & 0xFF00 ) >> 8 );
}

constexpr inline float reverse( float data )
{
	const uint32_t gg = reverse( ( uint32_t& )data );
	return ( float& )gg;
}

void Art01Sensor::parseTimerResponse()
{
	data.lock();
	io->read( nullptr, 5, TIME_IMMEDIATE );
	{
		struct
		{
			int8_t tmp[4];
            uint8_t status;
		} low;
		io->read( ( uint8_t* )&low, sizeof( low ), TIME_IMMEDIATE );
		for( auto i = 0; i < 4; ++i )
            data.instant.tmp[i] = ( int16_t )( low.tmp[i] );
    }
	io->read( nullptr, io->readAvailable(), TIME_IMMEDIATE );
	data.unlock();
}

template < typename T >
uint8_t Art01Sensor::checksum( T begin, T end ) const
{
	uint8_t s = 0;
	while( begin != end )
		s += *begin++;

	return s;
}
