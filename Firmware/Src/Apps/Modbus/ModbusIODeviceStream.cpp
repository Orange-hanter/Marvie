#include "ModbusIODeviceStream.h"

int ModbusIODeviceStream::read( uint8_t* buffer, size_t buffer_size )
{
	uint32_t a = io->readAvailable();
	if( a == 0 )
		return a;

	if( buffer_size != ( size_t )-1 && a > buffer_size )
		a = buffer_size;

	if( !buffer )
	{
		io->read( nullptr, a, TIME_INFINITE );
		return a;
	}

	return io->read( buffer, a, TIME_INFINITE );
}

int ModbusIODeviceStream::write( uint8_t* buffer, size_t len )
{
	return io->write( buffer, len, TIME_IMMEDIATE );
}

void ModbusIODeviceStream::txEnable( bool state )
{

}

bool ModbusIODeviceStream::writeComplete()
{
	return io->waitForBytesWritten( TIME_IMMEDIATE );
}

void ModbusIODeviceStream::communicationStatus( bool rx, bool tx )
{

}
