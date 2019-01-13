#include "Tem104USensor.h"
#include "Core/DateTimeService.h"

const char Tem104USensor::Name[] = "TEM104U";

Tem104USensor::Tem104USensor()
{
	address = 0;
	baudrate = 9600;
}

Tem104USensor::~Tem104USensor()
{

}

const char* Tem104USensor::name() const
{
	return Name;
}

void Tem104USensor::setAddress( uint8_t address )
{
	this->address = address;
}

void Tem104USensor::setBaudrate( uint32_t baudrate )
{
	this->baudrate = baudrate;
}

Tem104USensor::Data* Tem104USensor::readData()
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

	// read integrated values
	//                                SIG    ADDR          !ADDR         CGPR   CMD   LEN  ADDRH ADDRL TLEN   CS
	uint8_t readTimerRequest[10] = { 0x55, address, uint8_t( ~address ), 0x0F, 0x01, 0x03, 0x01, 0x44, 0x38, 0x42 };
	readTimerRequest[9] = checksum( readTimerRequest, readTimerRequest + 9 );
	io->write( readTimerRequest, sizeof( readTimerRequest ) );
	auto err = waitResponse( 0x38 );
	returnIfError( err, 1 );
	parseTimerResponse();

	// read instant values
	uint8_t readRAMRequestCh[10] = { 0x55, address, uint8_t( ~address ), 0x0C, 0x01, 0x03, 0x00, 0xB8, 0x18, 0x42 };
	readRAMRequestCh[9] = checksum( readRAMRequestCh, readRAMRequestCh + 9 );
	io->write( readRAMRequestCh, sizeof( readRAMRequestCh ) );
	err = waitResponse( 0x18 );
	returnIfError( err, 2 );
	parseRamResponse();

	data.lock();
	data.t = DateTimeService::currentDateTime();
	data.errType = SensorData::Error::NoError;
	data.unlock();
	io->read( nullptr, io->readAvailable() );

	io->releaseDevice();
	return &data;
}

Tem104USensor::Data* Tem104USensor::sensorData()
{
	return &data;
}

uint32_t Tem104USensor::sensorDataSize()
{
	return sizeof( Data ) - sizeof( SensorData );
}

SensorData::Error Tem104USensor::waitResponse( uint32_t size )
{
	if( !waitForReadAvailable( 6 + size + 1, TIME_MS2I( 2000 ) ) )
		return SensorData::Error::NoResponseError;
	uint8_t addr[2], len;
	io->peek( 1, addr, 2 );
	io->peek( 5, &len, 1 );
	if( addr[0] != address || addr[1] != uint8_t( ~address ) || len != size )
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

void Tem104USensor::parseTimerResponse()
{
	data.lock();
	io->read( nullptr, 6, TIME_IMMEDIATE );
	{
		struct
		{
			uint32_t VH;
			float VL;
			uint32_t MH;
			float ML;
			uint32_t EH;
			float EL;
		} pack;
		io->read( ( uint8_t* )&pack, sizeof( pack ), TIME_IMMEDIATE );
		data.integrated.intV = ( double )( reverse( pack.VH ) ) + ( double )( reverse( pack.VL ) );
		data.integrated.intM = ( double )( reverse( pack.MH ) ) + ( double )( reverse( pack.ML ) );
		data.integrated.intQ = ( double )( reverse( pack.EH ) ) + ( double )( reverse( pack.EL ) );
	}
	{
		struct
		{
			uint32_t T_Wrk;
			uint32_t T_Cnt;
			uint32_t T_Fail;
			uint32_t T_Dt;
			uint32_t T_Gmax;
			uint32_t T_Gmin;

		} pack;
		io->read( ( uint8_t* )&pack, sizeof( pack ), TIME_IMMEDIATE );
		data.integrated.T_Wrk	= reverse( pack.T_Wrk  );
		data.integrated.T_Cnt	= reverse( pack.T_Cnt  );
		data.integrated.T_Fail	= reverse( pack.T_Fail );
		data.integrated.T_Dt	= reverse( pack.T_Dt   );
		data.integrated.T_Gmax	= reverse( pack.T_Gmax );
		data.integrated.T_Gmin	= reverse( pack.T_Gmin );
	}
	{
		struct
		{
			uint8_t TekErr;
			uint8_t TehErr;
		} pack;
		io->read( ( uint8_t* )&pack, sizeof( pack ), TIME_IMMEDIATE );
		data.integrated.TekErr = pack.TekErr;
		data.integrated.TehErr = pack.TehErr;
	}
	{
		struct
		{
			uint16_t tmp[2];
			uint8_t prs[2];
		} pack;
		io->read( ( uint8_t* )&pack, sizeof( pack ), TIME_IMMEDIATE );
		for( auto i = 0; i < 2; ++i )
		{
			data.integrated.tmp[i] = reverse( pack.tmp[i] );
			data.integrated.prs[i] = pack.prs[i];
		}
	}
	io->read( nullptr, io->readAvailable(), TIME_IMMEDIATE );
	data.unlock();
}

void Tem104USensor::parseRamResponse()
{
	data.lock();
	io->read( nullptr, 6, TIME_IMMEDIATE );
	{
		struct
		{
			float rshv;
			float rshm;
			float tmp[2];
			float prs[2];
		} ram;
		io->read( ( uint8_t* )&ram, sizeof( ram ), TIME_IMMEDIATE );
		data.instant.tmp[0] = reverse( ram.tmp[0] );
		data.instant.tmp[1] = reverse( ram.tmp[1] );
		data.instant.prs[0] = reverse( ram.prs[0] ); 
		data.instant.prs[1] = reverse( ram.prs[1] ); 
		data.instant.rshv = reverse( ram.rshv );
		data.instant.rshm = reverse( ram.rshm );
	}
	io->read( nullptr, io->readAvailable(), TIME_IMMEDIATE );
	data.unlock();
}

template < typename T >
uint8_t Tem104USensor::checksum( T begin, T end ) const
{
	uint8_t s = 0;
	while( begin != end )
		s += *begin++;

	return ~s;
}
