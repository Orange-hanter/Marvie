#pragma once

#include "ModbusDevice.h"
#include "ModbusIODeviceStream.h"

class RawModbusClient : public ModbusDevice
{
public:
	RawModbusClient();
	~RawModbusClient();

	void setFrameType( FrameType type ) final override;
	inline void setIODevice( IODevice* io ) { stream.io = io; }

	virtual bool readHoldingRegisters( uint8_t slave, uint16_t address, size_t count, uint16_t* data );
	virtual bool readInputRegisters( uint8_t slave, uint16_t address, size_t count, uint16_t* data );
	virtual bool writeSingleRegisters( uint8_t slave, uint16_t address, uint16_t data );
	virtual bool writeMultipleRegisters( uint8_t slave, uint16_t address, size_t count, const uint16_t* data );

	inline ModbusDevice::Error error() { return err; }

protected:
	bool waitForResponse();
	static void timerCallback( void* p );

protected:
	FrameType frameType;
	ModbusIODeviceStream stream;
	uint8_t* buffer;
	ModbusPotato::IFramer* framer;
	ModbusPotato::CModbusMaster master;
	uint16_t reqAddr;
	size_t reqCount;
	uint16_t* reqData;
	struct MasterHandler : public ModbusPotato::IMasterHandler
	{
		RawModbusClient* client;
		MasterHandler( RawModbusClient* client ) : client( client ) {}
		virtual bool read_holding_registers_rsp( uint16_t address, size_t count, const uint16_t* data );
		virtual bool read_input_registers_rsp( uint16_t address, size_t count, const uint16_t* data );
		virtual bool write_single_register_rsp( uint16_t address );
		virtual bool write_multiple_registers_rsp( uint16_t address, size_t count );
		virtual bool response_time_out();
		virtual bool exception_response( ModbusPotato::modbus_exception_code::modbus_exception_code code );
	} masterHandler;
	friend class MasterHandler;
	Error err;
	volatile bool waitingResp;
};