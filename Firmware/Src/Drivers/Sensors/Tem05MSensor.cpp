#include "Tem05MSensor.h"
#include "Core/DateTimeService.h"

#define _V3( p ) ( *( uint32_t* )( p )&0x00FFFFFF )
#define _V2( p ) ( *( uint16_t* )( p ) )
#define BDC_TO_BIN( a ) ( ( ( a )&0x0F ) + ( ( a )&0xF0 ) * 10 )

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

const static uint8_t pipeDiameterList[8] = 
{ 
	10, 15, 25, 50, 
	80, 100, 150, 32
 };

 const static float GmaxList[24] = 
 {
	 0.25f, 0.5f, 1.0f, 1.25f,
	 2.5f, 5.0f, 2.5f, 5.0f,
	 10.0f, 10.0f, 20.0f, 40.0f,
	 25.0f, 50.0f, 100.0f, 50.0f,
	 100.0f, 200.0f, 100.0f, 200.0f,
	 400.0f, 5.0f, 10.0f, 20.0f,
 };

const char Tem05MSensor::Name[] = "TEM05M";

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
#define returnError( err, code )                  \
	{                                             \
		data.lock();                              \
		data.errType = err;                       \
		data.errCode = code;                      \
		data.unlock();                            \
		io->releaseDevice();                      \
		io->read( nullptr, io->readAvailable() ); \
		return &data;                             \
	}

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
	uint8_t head[11], tmp[37];
	io->peek( 0x0011, head, 11 );
	io->peek( 0x0020, tmp, 31 );
	io->peek( 0x00CE, tmp + 31, 6 );
	io->read( nullptr, 344 );

	const uint8_t& codeA = head[1]; // 0x0012
	const uint8_t& codeB = head[2]; // 0x0013
	if( codeA >= 24 || codeB >= 24 )
		return false;

	data.lock();

	data.t = DateTimeService::currentDateTime();
	data.errType = SensorData::Error::NoError;

	data.scheme = head[0]; // 0x0011
	data.pipeDiameter[0] = pipeDiameterList[codeA / 3];
	data.pipeDiameter[1] = pipeDiameterList[codeB / 3];
	data.ch[0].Gmax = GmaxList[codeA];
	data.ch[1].Gmax = GmaxList[codeB];
	data.ch[0].Gmin = ( ( head[3] >> 0 ) & 0x0F ) * data.ch[0].Gmax * 0.01f; // 0x0014
	data.ch[1].Gmin = ( ( head[3] >> 4 ) & 0x0F ) * data.ch[1].Gmax * 0.01f; // 0x0014
	data.t3 = head[4] * 0.1f; // 0x0015
	data.workingTime = _V3( head + 8 ) * 60 + head[7]; // 0x0018 - 0x001C

	// 0x0020 - 0x002F
	data.ch[0].G = _V3( tmp + 0 ) * multipliers[table[codeA].G];
	data.ch[0].P = _V3( tmp + 3 ) * multipliers[table[codeA].P];
	data.ch[0].Q = _V3( tmp + 6 ) * multipliers[table[codeA].Q];
	data.ch[0].V = _V3( tmp + 9 ) * multipliers[table[codeA].V];
	data.ch[0].M = _V3( tmp + 12 ) * multipliers[table[codeA].M];

	// 0x002F - 0x003E
	data.ch[1].G = _V3( tmp + 15 ) * multipliers[table[codeB].G];
	data.ch[1].P = _V3( tmp + 18 ) * multipliers[table[codeB].P];
	data.ch[1].Q = _V3( tmp + 21 ) * multipliers[table[codeB].Q];
	data.ch[1].V = _V3( tmp + 24 ) * multipliers[table[codeB].V];
	data.ch[1].M = _V3( tmp + 27 ) * multipliers[table[codeB].M];

	// 0x003E
	data.error = tmp[30];

	// 0x00CE - 0x00D4
	data.ch[0].t = _V2( tmp + 31 ) * 0.01f;
	data.ch[1].t = _V2( tmp + 33 ) * 0.01f;
	data.tCW = _V2( tmp + 35 ) * 0.01f;

	data.unlock();
	return true;
}
