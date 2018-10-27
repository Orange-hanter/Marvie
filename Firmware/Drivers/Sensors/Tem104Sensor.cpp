#include "Tem104Sensor.h"
#include "Core/DateTimeService.h"

const char Tem104Sensor::Name[] = "TEM104";

Tem104Sensor::Tem104Sensor()
{
	address = 0;
	baudrate = 9600;
}

Tem104Sensor::~Tem104Sensor()
{

}

const char* Tem104Sensor::name() const
{
	return Name;
}

void Tem104Sensor::setAddress( uint8_t address )
{
	this->address = address;
}

void Tem104Sensor::setBaudrate( uint32_t baudrate )
{
	this->baudrate = baudrate;
}

Tem104Sensor::Data* Tem104Sensor::readData()
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
	//                                SIG    ADDR          !ADDR         CGPR   CMD   LEN  ADDRH ADDRL TLEN   CS
	uint8_t readTimerRequest[10] = { 0x55, address, uint8_t( ~address ), 0x0F, 0x01, 0x03, 0x02, 0x00, 0xFF, 0x42 };
	readTimerRequest[9] = checksum( readTimerRequest, readTimerRequest + 9 );
	io->write( readTimerRequest, sizeof( readTimerRequest ) );
	auto err = waitResponse( 255 );
	returnIfError( err, 1 );
	parseTimerResponse();

	// read integrated values
	for( uint16_t i = 0; i < 4; ++i )
	{
		uint16_t addr = 0x2200 + i * 0x92;
		uint8_t readRAMRequestCh[10] = { 0x55, address, uint8_t( ~address ), 0x0C, 0x01, 0x03, uint8_t( addr >> 8 ), uint8_t( addr ), 0x92, 0x42 };
		readRAMRequestCh[9] = checksum( readRAMRequestCh, readRAMRequestCh + 9 );
		io->write( readRAMRequestCh, sizeof( readRAMRequestCh ) );
		auto err = waitResponse( 146 );
		returnIfError( err, 2 + i );
		parseRamResponse( i );
	}

	data.lock();
	data.t = DateTimeService::currentDateTime();
	data.errType = SensorData::Error::NoError;
	data.unlock();
	io->read( nullptr, io->readAvailable() );

	io->releaseDevice();
	return &data;
}

Tem104Sensor::Data* Tem104Sensor::sensorData()
{
	return &data;
}

uint32_t Tem104Sensor::sensorDataSize()
{
	return sizeof( Data ) - sizeof( SensorData );
}

SensorData::Error Tem104Sensor::waitResponse( uint32_t size )
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

void Tem104Sensor::parseTimerResponse()
{
	data.lock();
	io->read( nullptr, 6, TIME_IMMEDIATE );
	io->read( &data.integrated.date[0], 8, TIME_IMMEDIATE );
	{
		struct
		{
			float V[4];
			float M[4];
			float Q[4];
		} low;
		io->read( ( uint8_t* )&low, sizeof( low ), TIME_IMMEDIATE );
		for( auto i = 0; i < 4; ++i )
		{
			data.integrated.ch[i].intV = ( double )( reverse( low.V[i] ) );
			data.integrated.ch[i].intM = ( double )( reverse( low.M[i] ) );
			data.integrated.ch[i].intQ = ( double )( reverse( low.Q[i] ) );
		}
	}
	{
		struct
		{
			uint32_t V[4];
			uint32_t M[4];
			uint32_t Q[4];
		} hi;
		io->read( ( uint8_t* )&hi, sizeof( hi ), TIME_IMMEDIATE );
		for( auto i = 0; i < 4; ++i )
		{
			data.integrated.ch[i].intV += ( double )( reverse( hi.V[i] ) );
			data.integrated.ch[i].intM += ( double )( reverse( hi.M[i] ) );
			data.integrated.ch[i].intQ += ( double )( reverse( hi.Q[i] ) );
		}
	}
	io->read( ( uint8_t* )&data.integrated.TRab, 4, TIME_IMMEDIATE );
	data.integrated.TRab = reverse( data.integrated.TRab );
	{
		struct
		{
			uint32_t TNar[4];
			uint32_t Tmin[4];
			uint32_t Tmax[4];
		} dataErr;
		io->read( ( uint8_t* )&dataErr, sizeof( dataErr ), TIME_IMMEDIATE );
		for( auto i = 0; i < 4; ++i )
		{
			data.integrated.ch[i].TNar = reverse( dataErr.TNar[i] );
			data.integrated.ch[i].Tmin = reverse( dataErr.Tmin[i] );
			data.integrated.ch[i].Tmax = reverse( dataErr.Tmax[i] );
		}
	}
	{
		struct
		{
			uint32_t Tdt[4];
			uint32_t Ttn[4];
			uint8_t tekerr[4];
		} dataErr;
		io->read( ( uint8_t* )&dataErr, sizeof( dataErr ), TIME_IMMEDIATE );
		for( auto i = 0; i < 4; ++i )
		{
			data.integrated.ch[i].Tdt = reverse( dataErr.Tdt[i] );
			data.integrated.ch[i].Ttn = reverse( dataErr.Ttn[i] );
			data.integrated.ch[i].tekerr = dataErr.tekerr[i];
		}
	}
	{
		struct
		{
			uint16_t teherr[4];
			uint16_t tmp[4][3];
			uint8_t p[4][3];
		} dataTmp;
		io->read( ( uint8_t* )&dataTmp, sizeof( dataTmp ), TIME_IMMEDIATE );

		for( auto i = 0; i < 4; ++i )
		{
			data.integrated.ch[i].teherr = reverse( dataTmp.teherr[i] );
			for( auto j = 0; j < 3; ++j )
			{
				data.integrated.ch[i].t[j] = reverse( dataTmp.tmp[i][j] );
				data.integrated.ch[i].p[j] = dataTmp.p[i][j];
			}
		}
	}
	for( auto i = 0; i < 4; ++i )
	{
		io->read( ( uint8_t* )&data.integrated.ch[i].rshv, 4, TIME_IMMEDIATE );
		data.integrated.ch[i].rshv = reverse( data.integrated.ch[i].rshv );
	}
	io->read( nullptr, io->readAvailable(), TIME_IMMEDIATE ); // fuck yea. Sempai takoy zanuda
	data.unlock();
}

void Tem104Sensor::parseRamResponse( uint32_t ch )
{
	data.lock();
	io->read( nullptr, 6, TIME_IMMEDIATE );
	{
		struct
		{
			float tmp[4];
			float prs[4];
			float ro[4];
		} ram;
		io->read( ( uint8_t* )&ram, sizeof( ram ), TIME_IMMEDIATE );
		for( auto i = 0; i < 4; ++i )
		{
			data.instant.ch[ch].tmp[i] = reverse( ram.tmp[i] );
			data.instant.ch[ch].prs[i] = reverse( ram.prs[i] ); // sempai takoy zabotliviy
			data.instant.ch[ch].ro[i] = reverse( ram.ro[i] );
		}
	}
	{
		struct
		{
			float hent[4];
			float rshv[4];
			float rshm[4];
		} ram;
		io->read( ( uint8_t* )&ram, sizeof( ram ), TIME_IMMEDIATE );
		for( auto i = 0; i < 4; ++i )
		{
			data.instant.ch[ch].hent[i] = reverse( ram.hent[i] );
			data.instant.ch[ch].rshv[i] = reverse( ram.rshv[i] ); // sempai takoy zabotliviy x2
			data.instant.ch[ch].rshm[i] = reverse( ram.rshm[i] );
		}
	}
	{
#pragma pack( push, 1 )
		struct
		{
			float pwr[4];
			uint8_t tekerr;
			uint16_t teherr;
		} ram;
#pragma pack( pop )
		io->read( ( uint8_t* )&ram, sizeof( ram ), TIME_IMMEDIATE );
		for( auto i = 0; i < 4; ++i )
			data.instant.ch[ch].pwr[i] = reverse( ram.pwr[i] );
		data.instant.ch[ch].tekerr = ram.tekerr;
		data.instant.ch[ch].teherr = reverse( ram.teherr );
	}
	io->read( nullptr, io->readAvailable(), TIME_IMMEDIATE );
	data.unlock();
}

template < typename T >
uint8_t Tem104Sensor::checksum( T begin, T end ) const
{
	uint8_t s = 0;
	while( begin != end )
		s += *begin++;

	return ~s;
}
