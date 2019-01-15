#pragma once

#include "Apps/Modbus/TcpModbusServer.h"
#include "Apps/Modbus/RawModbusServer.h"
#include "lwipthread.h"

namespace ModbusServerTest
{
	#define MODBUS_REG_COUNT 512
	using namespace ModbusPotato;

	class ModbusSlaveHandlerHolding : public ISlaveHandler
	{
	public:
		ModbusSlaveHandlerHolding( uint16_t* registers, uint32_t count ) : modbusRegisters( registers ), count( count )
		{

		}
		modbus_exception_code::modbus_exception_code read_holding_registers( uint16_t address, uint16_t count, uint16_t* result ) override
		{
			if( count > MODBUS_REG_COUNT || address >= MODBUS_REG_COUNT || ( size_t )( address + count ) > MODBUS_REG_COUNT )
				return modbus_exception_code::illegal_data_address;

			// copy the values
			for( ; count; address++, count-- )
				*result++ = modbusRegisters[address];

			return modbus_exception_code::ok;
		}

	private:
		uint16_t * modbusRegisters;
		uint32_t count;
	};

	void test()
	{
		lwipInit( nullptr );

		uint16_t* modbusRegisters = new uint16_t[512];
		for( int i = 0; i < 512; ++i )
			modbusRegisters[i] = i;
		ModbusSlaveHandlerHolding slaveHandler( modbusRegisters, 512 );

		palSetPadMode( GPIOA, 9, PAL_MODE_ALTERNATE( GPIO_AF_USART1 ) | PAL_STM32_OSPEED_HIGHEST );
		palSetPadMode( GPIOA, 10, PAL_MODE_ALTERNATE( GPIO_AF_USART1 ) | PAL_STM32_OSPEED_HIGHEST );
		Usart* usart = Usart::instance( &SD1 );
		static uint8_t ob[128];
		usart->setOutputBuffer( ob, 1 );
		usart->open();

		RawModbusServer server;
		server.setFrameType( ModbusDevice::FrameType::Rtu );
		server.setSlaveHandler( &slaveHandler );
		server.setIODevice( usart );
		server.startServer();

		/*TcpModbusServer server;
		server.setFrameType( ModbusDevice::FrameType::Ip );
		server.setSlaveHandler( &slaveHandler );
		server.setPort( 502 );
		server.setMaxClientsCount( 2 );
		server.startServer();*/

		thread_reference_t ref = nullptr;
		chSysLock();
		chThdSuspendS( &ref );
	}
}