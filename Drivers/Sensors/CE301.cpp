#include "CE301.h"
#include "Core/DataTimeService.h"
#include "Support/Utility.h"

const uint8_t CE301::deviceAddressRequestUnion[5] = { '/', '?', '!', '\r', '\n' };
const uint8_t CE301::programModeRequest[6] = { ACK, 0x30, 0x35, 0x31, '\r', '\n' };
const uint8_t CE301::tariffsDataRequest[13] = { SOH, 'R', '1', STX, 'E', 'T', '0', 'P', 'E', '(', ')', ETX, 0x37 };

CE301::CE301()
{
	io = nullptr;
	address = 0;
}

CE301::~CE301()
{

}

const char* CE301::name() const
{
	return "CE301";
}

void CE301::setAddress( uint8_t address )
{
	this->address = address;
}

void CE301::setIODevice( IODevice* io )
{
	this->io = io;
}

IODevice* CE301::ioDevice()
{
	return io;
}

CE301::Data* CE301::readData()
{
#define returnInvalid() { data.valid = false; return &data; }

	// Device address request
	if( address == 0 )
		io->write( deviceAddressRequestUnion, sizeof( deviceAddressRequestUnion ), TIME_INFINITE );
	else
		io->write( deviceAddressRequest, sizeof( deviceAddressRequest ), TIME_INFINITE );
	ByteRingIterator end = waitForResponse( io, "\r\n", 2, TIME_S2I( 1 ) );
	if( !end.isValid() || *io->inputBuffer()->begin() != '/' )
		returnInvalid();
	io->inputBuffer()->read( nullptr, io->readAvailable() );

	chThdSleepMilliseconds( 1000 );

	// Program mode request
	io->write( programModeRequest, sizeof( programModeRequest ), TIME_INFINITE );
	char tmp[1] = { ETX };
	end = waitForResponse( io, tmp, 1, TIME_S2I( 1 ) );
	if( !end.isValid() || *io->inputBuffer()->begin() != SOH )
		returnInvalid();
	io->inputBuffer()->read( nullptr, io->readAvailable() );

	chThdSleepMilliseconds( 1000 );

	// Tariffs data request
	io->write( tariffsDataRequest, sizeof( tariffsDataRequest ), TIME_INFINITE );
	end = waitForResponse( io, tmp, 1, TIME_S2I( 1 ) );
	ByteRingIterator it = ++io->inputBuffer()->begin();
	if( !end.isValid() || *io->inputBuffer()->begin() != STX || !isValidChecksum( it, end ) )
		returnInvalid();

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
	data.t = DataTimeService::currentDataTime();
	data.valid = true;
	data.unlock();
	io->inputBuffer()->read( nullptr, io->readAvailable() );

	return &data;
}

CE301::Data* CE301::sensorData()
{
	return &data;
}

bool CE301::isValidChecksum( ByteRingIterator begin, ByteRingIterator end ) const
{
	uint8_t sum = 0;
	while( begin != end )
		sum += *begin++;

	return ( sum & 0x7F ) == *end;
}