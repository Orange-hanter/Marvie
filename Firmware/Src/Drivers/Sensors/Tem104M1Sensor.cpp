#include "Tem104M1Sensor.h"
#include "Core/DateTimeService.h"

const char Tem104M1Sensor::Name[] = "TEM104M1";

Tem104M1Sensor::Tem104M1Sensor()
{
	address = 0;
	baudrate = 9600;
}

Tem104M1Sensor::~Tem104M1Sensor()
{

}

const char* Tem104M1Sensor::name() const
{
	return Name;
}

void Tem104M1Sensor::setAddress( uint8_t address )
{
	this->address = address;
}

void Tem104M1Sensor::setBaudrate( uint32_t baudrate )
{
	this->baudrate = baudrate;
}

Tem104M1Sensor::Data* Tem104M1Sensor::readData()
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
	uint8_t readTimerRequest[10] = { 0x55, address, uint8_t( ~address ), 0x0F, 0x01, 0x03, 0x01, 0x80, 0x57, 0x42 };
	readTimerRequest[9] = checksum( readTimerRequest, readTimerRequest + 9 );
	io->write( readTimerRequest, sizeof( readTimerRequest ) );
	auto err = waitResponse( 0x57 );
	returnIfError( err, 1 );
	parseTimerResponse();

	// read instant values
	uint8_t readRAMRequestCh[10] = { 0x55, address, uint8_t( ~address ), 0x0C, 0x01, 0x03, 0x00, 0x00, 0x2f, 0x42 };
	readRAMRequestCh[9] = checksum( readRAMRequestCh, readRAMRequestCh + 9 );
	io->write( readRAMRequestCh, sizeof( readRAMRequestCh ) );
	err = waitResponse( 0x2F );
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

Tem104M1Sensor::Data* Tem104M1Sensor::sensorData()
{
	return &data;
}

uint32_t Tem104M1Sensor::sensorDataSize()
{
	return sizeof( Data ) - sizeof( SensorData );
}

SensorData::Error Tem104M1Sensor::waitResponse( uint32_t size )
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

void Tem104M1Sensor::parseTimerResponse()
{
	data.lock();
	io->read( nullptr, 6 + 8, TIME_IMMEDIATE );
	{
		struct
		{
			uint32_t VH;
			uint32_t MH;
			uint32_t EH;
			uint32_t EH_err;
			float VL;
			float ML;
			float EL;
			float EL_err;
		} pack;
		io->read( ( uint8_t* )&pack, sizeof( pack ), TIME_IMMEDIATE );
		data.integrated.intV = ( double )( pack.VH ) + ( double )( pack.VL );
		data.integrated.intM = ( double )( pack.MH ) + ( double )( pack.ML );
		data.integrated.intQ = ( double )( pack.EH ) + ( double )( pack.EL );
	}
	{
		struct
		{
			uint32_t T_Rab;
			uint32_t T_offline;
			uint32_t T_Nar;
			uint32_t T_Min;
			uint32_t T_Max;
			uint32_t T_Dt;
			uint32_t T_tn;
			uint32_t T_pt;

		} pack;
		io->read( ( uint8_t* )&pack, sizeof( pack ), TIME_IMMEDIATE );
		data.integrated.T_Rab = pack.T_Rab;
		data.integrated.T_Nar = pack.T_Nar;
		data.integrated.T_offline = pack.T_offline;
		data.integrated.T_Fail = pack.T_tn;
		data.integrated.T_Dt = pack.T_Dt;
		data.integrated.T_Gmax = pack.T_Max;
		data.integrated.T_Gmin = pack.T_Min;
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
			data.integrated.tmp[i] = pack.tmp[i];
			data.integrated.prs[i] = pack.prs[i];
		}
	}
	io->read( nullptr, io->readAvailable(), TIME_IMMEDIATE );
	data.unlock();
}

void Tem104M1Sensor::parseRamResponse()
{
	data.lock();
	io->read( nullptr, 6, TIME_IMMEDIATE );
	{
		struct
		{
			float tmp[2];
			float prs[2];
			float ro[2];
			float hent[2];

		} ram;
		io->read( ( uint8_t* )&ram, sizeof( ram ), TIME_IMMEDIATE );
		data.instant.tmp[0] = ram.tmp[0];
		data.instant.tmp[1] = ram.tmp[1];
		data.instant.prs[0] = ram.prs[0];
		data.instant.prs[1] = ram.prs[1];
	}
	{
		struct
		{
			float rshv;
			float rshm;
			float pwr;
		} ram;
		io->read( ( uint8_t* )&ram, sizeof( ram ), TIME_IMMEDIATE );
		data.instant.rshv = ram.rshv;
		data.instant.rshm = ram.rshm;
	}
	io->read( nullptr, io->readAvailable(), TIME_IMMEDIATE );
	data.unlock();
}

template < typename T >
uint8_t Tem104M1Sensor::checksum( T begin, T end ) const
{
	uint8_t s = 0;
	while( begin != end )
		s += *begin++;

	return ~s;
}
