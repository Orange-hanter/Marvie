#pragma once

#include "ModbusServer.h"
#include "ModbusIODeviceStream.h"

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
	ModbusIODeviceStream stream;	
	uint8_t* buffer;
	ModbusPotato::IFramer* framer;	
};