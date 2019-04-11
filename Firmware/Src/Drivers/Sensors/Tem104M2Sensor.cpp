#include "Tem104M2Sensor.h"
#include "Core/DateTimeService.h"

const char Tem104M2Sensor::Name[] = "TEM104M2";

Tem104M2Sensor::Tem104M2Sensor()
{
	address = 0;
	baudrate = 9600;
}

Tem104M2Sensor::~Tem104M2Sensor()
{
}

const char* Tem104M2Sensor::name() const
{
	return Name;
}

void Tem104M2Sensor::setAddress( uint8_t address )
{
	this->address = address;
}

void Tem104M2Sensor::setBaudrate( uint32_t baudrate )
{
	this->baudrate = baudrate;
}

Tem104M2Sensor::Data* Tem104M2Sensor::readData()
{
	io->acquireDevice();
#define returnIfError( err, code )                                \
	{                                                         \
		if( err != SensorData::Error::NoError )           \
		{                                                 \
			data.lock();                              \
			data.errType = err;                       \
			data.errCode = code;                      \
			data.unlock();                            \
			io->releaseDevice();                      \
			io->read( nullptr, io->readAvailable() ); \
			return &data;                             \
		}                                                 \
	}

	if( io->isSerialDevice() )
	{
		UsartBasedDevice* serialDevice = static_cast< UsartBasedDevice* >( io );
		serialDevice->setDataFormat( UsartBasedDevice::B8N );
		serialDevice->setStopBits( UsartBasedDevice::S1 );
		serialDevice->setBaudRate( baudrate );
	}
	io->read( nullptr, io->readAvailable() );
	{
		// read integrated values
		//                                SIG    ADDR          !ADDR         CGPR   CMD   LEN  ADDRH ADDRL TLEN   CS
		uint8_t readTimerRequest[10] = { 0x55, address, uint8_t( ~address ), 0x0F, 0x01, 0x03, 0x08, 0x00, 0x48, 0x42 };
		readTimerRequest[9] = checksum( readTimerRequest, readTimerRequest + 9 );
		io->write( readTimerRequest, sizeof( readTimerRequest ) );
		auto err = waitResponse( 0x48 );
		returnIfError( err, 11 );
		parseTimerResponsePart1();
	}
	{
		// read integrated values
		//                                SIG    ADDR          !ADDR         CGPR   CMD   LEN  ADDRH ADDRL TLEN   CS
		uint8_t readTimerRequest[10] = { 0x55, address, uint8_t( ~address ), 0x0F, 0x01, 0x03, 0x08, 0x48, 0x40, 0x42 };
		readTimerRequest[9] = checksum( readTimerRequest, readTimerRequest + 9 );
		io->write( readTimerRequest, sizeof( readTimerRequest ) );
		auto err = waitResponse( 0x40 );
		returnIfError( err, 12 );
		parseTimerResponsePart11();
	}
	{
		// read integrated values
		//                                SIG    ADDR          !ADDR         CGPR   CMD   LEN  ADDRH ADDRL TLEN   CS
		uint8_t readTimerRequest[10] = { 0x55, address, uint8_t( ~address ), 0x0F, 0x01, 0x03, 0x08, 0x98, 0x48, 0x42 };
		readTimerRequest[9] = checksum( readTimerRequest, readTimerRequest + 9 );
		io->write( readTimerRequest, sizeof( readTimerRequest ) );
		auto err = waitResponse( 0x48 );
		returnIfError( err, 21 );
		parseTimerResponsePart2();
	}
	{
		// read integrated values
		//                                SIG    ADDR          !ADDR         CGPR   CMD   LEN  ADDRH ADDRL TLEN   CS
		uint8_t readTimerRequest[10] = { 0x55, address, uint8_t( ~address ), 0x0F, 0x01, 0x03, 0x08, 0xE0, 0x60, 0x42 };
		readTimerRequest[9] = checksum( readTimerRequest, readTimerRequest + 9 );
		io->write( readTimerRequest, sizeof( readTimerRequest ) );
		auto err = waitResponse( 0x60 );
		returnIfError( err, 22 );
		parseTimerResponsePart22();
	}
	{
		// read instant values
		uint8_t readRAMRequestCh[10] = { 0x55, address, uint8_t( ~address ), 0x0C, 0x01, 0x03, 0x00, 0x00, 0x73, 0x42 };
		readRAMRequestCh[9] = checksum( readRAMRequestCh, readRAMRequestCh + 9 );
		io->write( readRAMRequestCh, sizeof( readRAMRequestCh ) );
		auto err = waitResponse( 0x73 );
		returnIfError( err, 3 );
		parseRamResponse( 0 );
	}
	{
		// read instant values
		uint8_t readRAMRequestCh[10] = { 0x55, address, uint8_t( ~address ), 0x0C, 0x01, 0x03, 0x40, 0x73, 0x73, 0x42 };
		readRAMRequestCh[9] = checksum( readRAMRequestCh, readRAMRequestCh + 9 );
		io->write( readRAMRequestCh, sizeof( readRAMRequestCh ) );
		auto err = waitResponse( 0x73 );
		returnIfError( err, 4 );
		parseRamResponse( 1 );
	}
	{
		// read instant values
		uint8_t readRAMRequestCh[10] = { 0x55, address, uint8_t( ~address ), 0x0C, 0x01, 0x03, 0x40, 0xE6, 0x73, 0x42 };
		readRAMRequestCh[9] = checksum( readRAMRequestCh, readRAMRequestCh + 9 );
		io->write( readRAMRequestCh, sizeof( readRAMRequestCh ) );
		auto err = waitResponse( 0x73 );
		returnIfError( err, 5 );
		parseRamResponse( 2 );
	}
	{
		// read instant values
		uint8_t readRAMRequestCh[10] = { 0x55, address, uint8_t( ~address ), 0x0C, 0x01, 0x03, 0x41, 0x59, 0x73, 0x42 };
		readRAMRequestCh[9] = checksum( readRAMRequestCh, readRAMRequestCh + 9 );
		io->write( readRAMRequestCh, sizeof( readRAMRequestCh ) );
		auto err = waitResponse( 0x73 );
		returnIfError( err, 6 );
		parseRamResponse( 3 );
	}

	data.lock();
	data.t = DateTimeService::currentDateTime();
	data.errType = SensorData::Error::NoError;
	data.unlock();
	io->read( nullptr, io->readAvailable() );

	io->releaseDevice();
	return &data;
}

Tem104M2Sensor::Data* Tem104M2Sensor::sensorData()
{
	return &data;
}

uint32_t Tem104M2Sensor::sensorDataSize()
{
	return sizeof( Data ) - sizeof( SensorData );
}

SensorData::Error Tem104M2Sensor::waitResponse( uint32_t size )
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

void Tem104M2Sensor::parseTimerResponsePart1()
{
	data.lock();
	io->read( nullptr, 6 + 8, TIME_IMMEDIATE );
	{
		struct
		{
			uint32_t VH[4];
			uint32_t MH[4];
			uint32_t EH[4];
			uint32_t EH_err[4];

		} pack;
		io->read( ( uint8_t* )&pack, sizeof( pack ), TIME_IMMEDIATE );
		for( size_t i = 0; i < 4; i++ )
		{
			data.integrated.ch[i].intV = ( double )( pack.VH[i] );
			data.integrated.ch[i].intM = ( double )( pack.MH[i] );
			data.integrated.ch[i].intQ = ( double )( pack.EH[i] );
			data.integrated.ch[i].intQerr = ( double )( pack.EH_err[i] );
		}
	}
	io->read( nullptr, io->readAvailable(), TIME_IMMEDIATE );
	data.unlock();
}

void Tem104M2Sensor::parseTimerResponsePart11()
{
	data.lock();
	io->read( nullptr, 6, TIME_IMMEDIATE );
	{
		struct
		{
			float VL[4];
			float ML[4];
			float EL[4];
			float EL_err[4];

		} pack;
		io->read( ( uint8_t* )&pack, sizeof( pack ), TIME_IMMEDIATE );
		for( size_t i = 0; i < 4; i++ )
		{
			data.integrated.ch[i].intV += ( double )( pack.VL[i] );
			data.integrated.ch[i].intM += ( double )( pack.ML[i] );
			data.integrated.ch[i].intQ += ( double )( pack.EL[i] );
			data.integrated.ch[i].intQerr += ( double )( pack.EL_err[i] );
		}
	}
	io->read( nullptr, io->readAvailable(), TIME_IMMEDIATE );
	data.unlock();
}

void Tem104M2Sensor::parseTimerResponsePart2()
{
	data.lock();
	io->read( nullptr, 6, TIME_IMMEDIATE );
	{
		struct
		{
			uint32_t T_Rab;
			uint32_t T_offline;
			uint32_t T_Nar[4];
			uint32_t T_Min[4];
			uint32_t T_Max[4];
		} pack;
		io->read( ( uint8_t* )&pack, sizeof( pack ), TIME_IMMEDIATE );
		for( size_t i = 0; i < 4; i++ )
		{
			data.integrated.T_Rab = pack.T_Rab;
			data.integrated.T_offline = pack.T_offline;
			data.integrated.ch[i].T_Nar = pack.T_Nar[i];
			data.integrated.ch[i].T_Min = pack.T_Min[i];
			data.integrated.ch[i].T_Max = pack.T_Max[i];
		}
	}
	{
		struct
		{
			uint32_t T_Dt[4];
		} pack;
		io->read( ( uint8_t* )&pack, sizeof( pack ), TIME_IMMEDIATE );
		for( size_t i = 0; i < 4; i++ )
		{
			data.integrated.ch[i].T_Dt = pack.T_Dt[i];
		}
	}
	
	io->read( nullptr, io->readAvailable(), TIME_IMMEDIATE );
	data.unlock();
}

void Tem104M2Sensor::parseTimerResponsePart22()
{
	data.lock();
	io->read( nullptr, 6, TIME_IMMEDIATE );
	{
		struct
		{
			uint32_t T_tn[4];
			uint32_t T_rev[4];
			uint32_t T_pt[4];
		} pack;
		io->read( ( uint8_t* )&pack, sizeof( pack ), TIME_IMMEDIATE );
		for( size_t i = 0; i < 4; i++ )
		{
			data.integrated.ch[i].T_tn = pack.T_tn[i];
			data.integrated.ch[i].T_rev = pack.T_rev[i];
			data.integrated.ch[i].T_pt = pack.T_pt[i];
		}
	}
	{
		struct
		{
			uint8_t TekErr[4];
			uint8_t TehErr[4];
		} pack;
		io->read( ( uint8_t* )&pack, sizeof( pack ), TIME_IMMEDIATE );
		for( size_t i = 0; i < 4; i++ )
		{
			data.integrated.ch[i].TekErr = pack.TekErr[i];
			data.integrated.ch[i].TehErr = pack.TehErr[i];
		}
	}
	{
		struct
		{
			uint16_t tmp[4][3];
			uint8_t prs[4][3];
		} pack;
		io->read( ( uint8_t* )&pack, sizeof( pack ), TIME_IMMEDIATE );
		for( auto i = 0; i < 4; ++i )
			for( auto j = 0; j < 4; ++j )
			{
				data.integrated.ch[i].tmp[i] = pack.tmp[i][j];
				data.integrated.ch[i].prs[i] = pack.prs[i][j];
			}
	}

	io->read( nullptr, io->readAvailable(), TIME_IMMEDIATE );
	data.unlock();
}

void Tem104M2Sensor::parseRamResponse( uint16_t chanel )
{
	data.lock();
	io->read( nullptr, 6, TIME_IMMEDIATE );
	{
		struct
		{
			float tmp[4];
			float prs[4];
			float ro[4];
			float hent[4];
		} ram;
		io->read( ( uint8_t* )&ram, sizeof( ram ), TIME_IMMEDIATE );
		for( size_t i = 0; i < 4; i++ )
		{
			data.instant.ch[chanel].tmp[i] = ram.tmp[i];
			data.instant.ch[chanel].prs[i] = ram.prs[i];
			data.instant.ch[chanel].ro[i] = ram.ro[i];
			data.instant.ch[chanel].hent[i] = ram.hent[i];
		}
	}
	{
		struct
		{
			float rshv[4];
			float rshm[4];
			float pwr[4];
		} ram;
		io->read( ( uint8_t* )&ram, sizeof( ram ), TIME_IMMEDIATE );
		for( size_t i = 0; i < 4; i++ )
		{
			data.instant.ch[chanel].rshv[i] = ram.rshv[i];
			data.instant.ch[chanel].rshm[i] = ram.rshm[i];
			data.instant.ch[chanel].pwr[i] = ram.pwr[i];
		}
	}
	io->read( nullptr, io->readAvailable(), TIME_IMMEDIATE );
	data.unlock();
}

template< typename T >
uint8_t Tem104M2Sensor::checksum( T begin, T end ) const
{
	uint8_t s = 0;
	while( begin != end )
		s += *begin++;

	return ~s;
}
