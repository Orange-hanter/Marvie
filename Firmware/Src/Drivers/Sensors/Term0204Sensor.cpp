#include "Term0204Sensor.h"
#include "Core/DateTimeService.h"

const char TERM0204Sensor::Name[] = "TERM0204";

TERM0204Sensor::TERM0204Sensor()
{
	address = 0;
	baudrate = 9600;
}

TERM0204Sensor::~TERM0204Sensor()
{

}

const char* TERM0204Sensor::name() const
{
	//TODO: add to end serial nubber of senssor
	return Name;
}

void TERM0204Sensor::setAddress( uint16_t address )
{
	this->address = address;
}

void TERM0204Sensor::setBaudrate( uint32_t baudrate )
{
	this->baudrate = baudrate;
}

TERM0204Sensor::Data* TERM0204Sensor::readData()
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

	// read all data			  
	uint8_t readTimerRequest[6] = { 0x1d, 0x1b, 0x30, 0x30, 0x30, 0x30 };
	readTimerRequest[2] |= ( address >> 12 ) & 0x0F;
	readTimerRequest[3] |= ( address >> 8 ) & 0x0F;
	readTimerRequest[4] |= ( address >> 4 ) & 0x0F;
	readTimerRequest[5] |= address & 0x0F;

	io->write( readTimerRequest, sizeof( readTimerRequest ) );
	auto err = waitResponse( 153 );
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

TERM0204Sensor::Data* TERM0204Sensor::sensorData()
{
	return &data;
}

uint32_t TERM0204Sensor::sensorDataSize()
{
	return sizeof( Data ) - sizeof( SensorData );
}

SensorData::Error TERM0204Sensor::waitResponse( uint32_t size )
{
	if( !waitForReadAvailable( size, TIME_MS2I( 2000 ) ) )
		return SensorData::Error::NoResponseError;
	uint8_t startByte;
	io->peek( 0, &startByte, 1 );
	if( startByte != 0x07 )
		return SensorData::Error::CrcError;	// TODO: ��������� ������ ��� ������
	/* ��� ��� ����������� �����
	uint8_t cs = checksum() ,
		csSensor = toByte( *( io->inputBuffer()->begin() + ( 1 + 152 - 2 ) ), *( io->inputBuffer()->begin() + ( 1 + 152 - 1 ) ) );
	if( cs != csSensor )
		return SensorData::Error::CrcError;*/

	return SensorData::Error::NoError;
}

void TERM0204Sensor::parseTimerResponse()
{
	data.lock();
	uint8_t buf[66];

	io->read( buf, 5 );
	data.instantValues.error = ( toByte( buf + 1 ) << 8 ) | toByte( buf + 3 );

	io->read( buf, 66 );
	data.instantValues.ch[0].G = b3ToFloat( buf ) * 3600.0f;
	data.instantValues.ch[1].G = b3ToFloat( buf + 6 ) * 3600.0f;
	data.instantValues.ch[0].P = b3ToFloat( buf + 12 ) * 3600.0f;
	data.instantValues.ch[1].P = b3ToFloat( buf + 18 ) * 3600.0f;
	data.instantValues.ch[0].tmpSupply = b3ToFloat( buf + 24 );
	data.instantValues.ch[0].tmpReturn = b3ToFloat( buf + 30 );
	data.instantValues.ch[1].tmpSupply = b3ToFloat( buf + 36 );
	data.instantValues.ch[1].tmpReturn = b3ToFloat( buf + 42 );
	data.instantValues.pSupply = b3ToFloat( buf + 48 );
	data.instantValues.pReturn = b3ToFloat( buf + 54 );
	data.instantValues.vAddition = b3ToFloat( buf + 60 );

	io->read( nullptr, 12 );
	io->read( buf, 56 );
	data.integratedValues.ch[0].vSumm = b4ToFloat( buf );
	data.integratedValues.ch[1].vSumm = b4ToFloat( buf + 8 );
	data.integratedValues.ch[0].mSumm = b4ToFloat( buf + 16 );
	data.integratedValues.ch[1].mSumm = b4ToFloat( buf + 24 );
	data.integratedValues.ch[0].QSumm = b4ToFloat( buf + 32 );
	data.integratedValues.ch[1].QSumm = b4ToFloat( buf + 40 );
	data.integratedValues.vSummAddition = b4ToFloat( buf + 48 );

	io->read( nullptr, io->readAvailable(), TIME_IMMEDIATE );
	data.unlock();
}

template < typename T >
uint8_t TERM0204Sensor::checksum( T begin, T end ) const
{
	uint8_t s = 0;
	while( begin != end )
		s += toByte( *begin++, *begin++ );	//TODO: ��������� �����

	return s;
}

inline uint8_t TERM0204Sensor::toByte( ByteRingIterator& i ) const
{
	char cHigh = *i++, cLow = *i++;
	return ( ( cHigh - 0x30 ) << 4 ) | ( cLow - 0x30 );
}

uint8_t TERM0204Sensor::toByte( char cHigh, char cLow ) const
{
	return ( ( cHigh - 0x30 ) << 4 ) | ( cLow - 0x30 );
}

uint8_t TERM0204Sensor::toByte( const uint8_t* p ) const
{
	return ( ( p[0] - 0x30 ) << 4 ) | ( p[1] - 0x30 );
}

float TERM0204Sensor::b3ToFloat( uint8_t b0, uint8_t b1, uint8_t b2 )
{
	uint16_t m = ( b1 << 8 ) | b2;
	if( m == 0 )
		return 0.0f;
	uint32_t v = 0;
	reinterpret_cast< uint8_t* >( &v )[3] |= b0 & 0x80;
	b0 &= 0x7F;
	b0 = 0x7F - 0x40 + b0;
	while( !( m & 0x8000 ) )
		m <<= 1, --b0;
	m <<= 1, --b0;
	v |= ( b0 << 23 ) | ( m << 7 );
	return *reinterpret_cast< float* >( &v );
}

float TERM0204Sensor::b4ToFloat( uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3 )
{
	uint32_t m = ( ( b1 << 16 ) | ( b2 << 8 ) | b3 ) >> 1;
	if( m == 0 )
		return 0.0f;
	volatile uint32_t v = 0;
	reinterpret_cast< uint8_t volatile* >( &v )[3] |= b0 & 0x80;
	b0 &= 0x7F;
	b0 = 0x7F - 0x40 + b0;
	while( !( m & 0x400000 ) )
		m <<= 1, --b0;
	m <<= 1, --b0;
	v |= ( b0 << 23 ) | ( m & 0x7FFFFF );
	return *reinterpret_cast< float volatile* >( &v );
}

float TERM0204Sensor::b3ToFloat( const uint8_t* p )
{
	return b3ToFloat( toByte( p ), toByte( p + 2 ), toByte( p + 4 ) );
}

float TERM0204Sensor::b4ToFloat( const uint8_t* p )
{
	return b4ToFloat( toByte( p ), toByte( p + 2 ), toByte( p + 4 ), toByte( p + 6 ) );
}
