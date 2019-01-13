#include "Mr01Sensor.h"
#include "Core/DateTimeService.h"

const char MR01Sensor::Name[] = "MR01";

MR01Sensor::MR01Sensor()
{
	address = 0;
	baudrate = 9600;
}

MR01Sensor::~MR01Sensor()
{

}

const char* MR01Sensor::name() const
{
	return Name;
}

void MR01Sensor::setAddress( uint16_t address )
{
	uint16_t digits[4];
	digits[3] = address % 10;
	digits[2] = ( address / 10 ) % 10;
	digits[1] = ( address / 100 ) % 10;
	digits[0] = ( address / 1000 );

	this->address = ( digits[0] << 12 ) | ( digits[1] << 8 ) | ( digits[2] << 4 ) | ( digits[3] << 0 );
}

void MR01Sensor::setBaudrate( uint32_t baudrate )
{
	this->baudrate = baudrate;
}

MR01Sensor::Data* MR01Sensor::readData()
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
	uint8_t readTimerRequest[5] = { 0x1B, 0x30, 0x30, 0x30, 0x30 };
	readTimerRequest[1] |= ( address >> 12 ) & 0x0F;
	readTimerRequest[2] |= ( address >> 8 ) & 0x0F;
	readTimerRequest[3] |= ( address >> 4 ) & 0x0F;
	readTimerRequest[4] |= address & 0x0F;

	io->write( readTimerRequest, sizeof( readTimerRequest ) );
	auto err = waitResponse( 20 );
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

MR01Sensor::Data* MR01Sensor::sensorData()
{
	return &data;
}

uint32_t MR01Sensor::sensorDataSize()
{
	return sizeof( Data ) - sizeof( SensorData );
}

SensorData::Error MR01Sensor::waitResponse( uint32_t size )
{
	if( !waitForReadAvailable( size, TIME_MS2I( 2000 ) ) )
		return SensorData::Error::NoResponseError;
	//hghg
	uint8_t cs = checksum( io->inputBuffer()->begin(), io->inputBuffer()->end() - 2 );
	uint8_t csDevive = ( ( *( io->inputBuffer()->end() - 2 ) - 0x30 ) << 4 ) | ( *( io->inputBuffer()->end() - 1 ) - 0x30 );

	if( cs != csDevive )
		return SensorData::Error::CrcError;

	return SensorData::Error::NoError;
}

void MR01Sensor::parseTimerResponse()
{
	data.lock();
	{
		uint8_t rawBytes[20];
		io->read( ( uint8_t* )&rawBytes, sizeof( rawBytes ), TIME_IMMEDIATE );
		for( auto i = 0, j = 0; i < 8; i++, j += 2 )
		{
			data.instant.tmp[i] = ( int8_t )( ( rawBytes[j] - 0x30 ) << 4 ) | ( rawBytes[j + 1] - 0x30 );
		}
		data.instant.status = ( uint16_t )( ( ( rawBytes[16] - 0x30 ) << 4 ) | ( rawBytes[17] - 0x30 ) );
	}
	io->read( nullptr, io->readAvailable(), TIME_IMMEDIATE );
	data.unlock();
}

template < typename T >
uint8_t MR01Sensor::checksum( T begin, T end ) const
{
	uint8_t s = 0;
	while( begin != end )
		s += ( ( *begin++ - 0x30 ) << 4 ) | ( *begin++ - 0x30 );

	return s;
}
