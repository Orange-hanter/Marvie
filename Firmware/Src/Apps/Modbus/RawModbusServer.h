#pragma once

#include "Core/IODevice.h"
#include "ModbusServer.h"

#define RAW_MODBUS_SERVER_STACK_SIZE  1024

class RawModbusServer : public AbstractModbusServer
{
public:
	RawModbusServer( uint32_t stackSize = RAW_MODBUS_SERVER_STACK_SIZE );
	~RawModbusServer();

	void setIODevice( IODevice* io );
	bool startServer( tprio_t prio = NORMALPRIO ) final override;

private:
	void main() final override;
	static void timerCallback( void* p );

private:
	struct Stream : public ModbusPotato::IStream
	{
		IODevice* io;
		Stream();

		int read( uint8_t* buffer, size_t buffer_size );
		int write( uint8_t* buffer, size_t len );
		void txEnable( bool state );
		bool writeComplete();
		void communicationStatus( bool rx, bool tx );
	} stream;	
	uint8_t* buffer;
	ModbusPotato::IFramer* framer;	
};