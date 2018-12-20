#include "Tem05MSensor.h"
#include "Core/DateTimeService.h"

#define _V3( p ) ( *( uint32_t* )( p ) & 0x00FFFFFF )
#define _V2( p ) ( *( uint16_t* )( p ) )
#define BDC_TO_BIN( a ) ( ( ( a ) & 0x0F ) + ( ( a ) & 0xF0 ) * 10 )

const static float multipliers[10] = { 1.0e-00f, 1.0e-01f, 1.0e-02f, 1.0e-03f, 1.0e-04f, 1.0e-05f, 1.0e-06f, 1.0e-07f, 1.0e-08f, 1.0e-09f };
struct MultipliersDegrees
{
	uint32_t G : 4,
			 P : 4,
			 Q : 4,
			 V : 4,
			 M : 4;
};

const static MultipliersDegrees table[24] =
{
	{ 5, 3, 4, 3, 3 },
	{ 5, 3, 4, 3, 3 },
	{ 4, 2, 3, 2, 2 },
	{ 4, 2, 3, 2, 2 },

	{ 4, 2, 3, 2, 2 },
	{ 4, 2, 3, 2, 2 },
	{ 4, 2, 3, 2, 2 },
	{ 4, 2, 3, 2, 2 },

	{ 3, 1, 2, 1, 1 },
	{ 3, 1, 2, 1, 1 },
	{ 3, 1, 2, 1, 1 },
	{ 3, 1, 2, 1, 1 },

	{ 3, 1, 2, 1, 1 },
	{ 3, 1, 2, 1, 1 },
	{ 2, 0, 1, 0, 0 },
	{ 3, 1, 2, 1, 1 },

	{ 2, 0, 1, 0, 0 },
	{ 2, 0, 1, 0, 0 },
	{ 2, 0, 1, 0, 0 },
	{ 2, 0, 1, 0, 0 },

	{ 2, 0, 1, 0, 0 },
	{ 4, 2, 3, 2, 2 },
	{ 3, 1, 2, 1, 1 },
	{ 3, 1, 2, 1, 1 },
};

const char Tem05MSensor::Name[] = "Tem05M";

Tem05MSensor::Tem05MSensor()
{
	baudrate = 9600;
}

Tem05MSensor::~Tem05MSensor()
{

}

const char* Tem05MSensor::name() const
{
	return Name;
}

void Tem05MSensor::setBaudrate( uint32_t baudrate )
{
	this->baudrate = baudrate;
}

Tem05MSensor::Data* Tem05MSensor::readData()
{
	io->acquireDevice();
#define returnError( err, code ) { data.lock(); data.errType = err; data.errCode = code; data.unlock(); io->releaseDevice(); io->read( nullptr, io->readAvailable() ); return &data; }

	if( io->isSerialDevice() )
	{
		UsartBasedDevice* serialDevice = static_cast< UsartBasedDevice* >( io );
		serialDevice->setDataFormat( UsartBasedDevice::B8N );
		serialDevice->setStopBits( UsartBasedDevice::S1 );
		serialDevice->setBaudRate( baudrate );
	}
	io->read( nullptr, io->readAvailable() );

	// alignment procedure
	while( waitForReadAvailable( 1, TIME_MS2I( 200 ) ) )
		io->read( nullptr, io->readAvailable() );

	if( !waitForReadAvailable( 1, TIME_MS2I( 10000 ) ) )
		returnError( SensorData::Error::NoResponseError, 1 );
	if( !waitForReadAvailable( 344, TIME_MS2I( 800 ) ) )
		returnError( SensorData::Error::NoResponseError, 2 );
	if( !parseData() )
		returnError( SensorData::Error::CrcError, 3 );

	io->releaseDevice();
	return &data;
}

Tem05MSensor::Data* Tem05MSensor::sensorData()
{
	return &data;
}

uint32_t Tem05MSensor::sensorDataSize()
{
	return sizeof( Data ) - sizeof( SensorData );
}

bool Tem05MSensor::parseData()
{
	uint8_t codeA, codeB, tmp[30];
	io->peek( 0x0012, &codeA, 1 );
	io->peek( 0x0013, &codeB, 1 );
	io->peek( 0x0020, tmp, 30 );
	io->read( nullptr, 344 );

	if( codeA >= 24 || codeB >= 24 )
		return false;

	data.lock();

	data.t = DateTimeService::currentDateTime();
	data.errType = SensorData::Error::NoError;

	// 0x0020 - 0x002F
	data.G[0] = _V3( tmp + 0 ) * multipliers[table[codeA].G];
	data.P[0] = _V3( tmp + 3 ) * multipliers[table[codeA].P];
	data.Q[0] = _V3( tmp + 6 ) * multipliers[table[codeA].Q];
	data.V[0] = _V3( tmp + 9 ) * multipliers[table[codeA].V];
	data.M[0] = _V3( tmp + 12 ) * multipliers[table[codeA].M];

	// 0x002F - 0x003E
	data.G[1] = _V3( tmp + 15 ) * multipliers[table[codeB].G];
	data.P[1] = _V3( tmp + 18 ) * multipliers[table[codeB].P];
	data.Q[1] = _V3( tmp + 21 ) * multipliers[table[codeB].Q];
	data.V[1] = _V3( tmp + 24 ) * multipliers[table[codeB].V];
	data.M[1] = _V3( tmp + 27 ) * multipliers[table[codeB].M];

	data.unlock();
	return true;
}