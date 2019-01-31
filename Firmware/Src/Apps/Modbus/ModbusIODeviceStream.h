#pragma once

#include "Core/IODevice.h"
#include "ModbusDevice.h"

class ModbusIODeviceStream : public ModbusPotato::IStream
{
public:
	IODevice * io;

	ModbusIODeviceStream( IODevice* io = nullptr ) : io( io ) {}

	int read( uint8_t* buffer, size_t buffer_size ) override;
	int write( uint8_t* buffer, size_t len ) override;
	void txEnable( bool state ) override;
	bool writeComplete() override;
	void communicationStatus( bool rx, bool tx ) override;
};