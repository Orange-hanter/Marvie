#include "CE301Sensor.h"
#include "Core/DateTimeService.h"
#include "Support/Utility.h"

const char CE301Sensor::Name[] = "CE301";

const uint8_t CE301Sensor::deviceAddressRequestUnion[5] = { '/', '?', '!', '\r', '\n' };
const uint8_t CE301Sensor::programModeRequest[6] = { ACK, 0x30, 0x35, 0x31, '\r', '\n' };
const uint8_t CE301Sensor::tariffsDataRequest[13] = { SOH, 'R', '1', STX, 'E', 'T', '0', 'P', 'E', '(', ')', ETX, 0x37 };

CE301Sensor::CE301Sensor()
{
	io = nullptr;
	address = 0;
	baudrate = 9600;
}

CE301Sensor::~CE301Sensor()
{

}

const char* CE301Sensor::name() const
{
	return Name;
}

void CE301Sensor::setAddress( uint8_t address )
{
	this->address = address;
}

void CE301Sensor::setBaudrate( uint32_t baudrate )
{
	this->baudrate = baudrate;
}

void CE301Sensor::setIODevice( IODevice* io )
{
	this->io = static_cast< UsartBasedDevice* >( io );
}

IODevice* CE301Sensor::ioDevice()
{
	return io;
}

CE301Sensor::Data* CE301Sensor::readData()
{
#define returnNoResponse( code ) { data.errType = SensorData::Error::NoResponseError; data.errCode = code; data.t = DateTimeService::currentDateTime(); return &data; }
#define returnCrcError( code ) { data.errType = SensorData::Error::CrcError; data.errCode = code; data.t = DateTimeService::currentDateTime(); return &data; }

	io->setDataFormat( UsartBasedDevice::B7E );
	io->setStopBits( UsartBasedDevice::S1 );
	io->setBaudRate( baudrate );

	// Device address request
	if( address == 0 )
		io->write( deviceAddressRequestUnion, sizeof( deviceAddressRequestUnion ), TIME_INFINITE );
	else
		io->write( deviceAddressRequest, sizeof( deviceAddressRequest ), TIME_INFINITE );
	ByteRingIterator end = waitForResponse( io, "\r\n", 2, TIME_S2I( 1 ) );
	if( !end.isValid() || *io->inputBuffer()->begin() != '/' )
		returnNoResponse( 0 );
	io->inputBuffer()->read( nullptr, io->readAvailable() );

	chThdSleepMilliseconds( 1000 );

	// Program mode request
	io->write( programModeRequest, sizeof( programModeRequest ), TIME_INFINITE );
	char tmp[1] = { ETX };
	end = waitForResponse( io, tmp, 1, TIME_S2I( 1 ) );
	if( !end.isValid() || *io->inputBuffer()->begin() != SOH )
		returnNoResponse( 1 );
	io->inputBuffer()->read( nullptr, io->readAvailable() );

	chThdSleepMilliseconds( 1000 );

	// Tariffs data request
	io->write( tariffsDataRequest, sizeof( tariffsDataRequest ), TIME_INFINITE );
	end = waitForResponse( io, tmp, 1, TIME_S2I( 1 ) );
	ByteRingIterator it = ++io->inputBuffer()->begin();
	if( !end.isValid() || *io->inputBuffer()->begin() != STX )
		returnNoResponse( 2 );
	if( !isValidChecksum( it, end ) )
		returnCrcError( 3 );

	// Parse tariff data
	data.lock();
	it += 6;
	for( int i = 0; i < 6; i++ )
	{
		data.tariffs[i] = Utility::parseDouble< double, ByteRingIterator >( it );
		while( *++it != '\n' )
			;
		it += 7;
	}
	data.t = DateTimeService::currentDateTime();
	data.errType = SensorData::Error::NoError;
	data.unlock();
	io->inputBuffer()->read( nullptr, io->readAvailable() );

	return &data;
}

CE301Sensor::Data* CE301Sensor::sensorData()
{
	return &data;
}

uint32_t CE301Sensor::sensorDataSize()
{
	return sizeof( Data ) - sizeof( SensorData );
}

bool CE301Sensor::isValidChecksum( ByteRingIterator begin, ByteRingIterator end ) const
{
	uint8_t sum = 0;
	while( begin != end )
		sum += *begin++;

	return ( sum & 0x7F ) == *end;
}