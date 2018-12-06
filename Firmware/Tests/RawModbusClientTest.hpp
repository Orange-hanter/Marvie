#pragma once

#include "Apps/Modbus/RawModbusClient.h"
#include "Drivers/Interfaces/Usart.h"
#include "Core/Assert.h"

namespace RawModbusClientTest
{
	int test()
	{
		uint8_t* ob = new uint8_t[1024];
		uint8_t* ib = new uint8_t[1024];
		memset( ob, 0, 1024 );
		memset( ib, 0, 1024 );

		//&SD1, IOPA10, IOPA9
		palSetPadMode( GPIOA, 10, PAL_MODE_ALTERNATE( GPIO_AF_USART1 ) );
		palSetPadMode( GPIOA, 9, PAL_MODE_ALTERNATE( GPIO_AF_USART1 ) );
		Usart* usart = Usart::instance( &SD1 );
		usart->setInputBuffer( ib, 1024 );
		usart->setOutputBuffer( ob, 1024 );
		usart->open( 115200 );

		/*while( true )
		{
			usart->write( ( uint8_t* )"Hello", 5, TIME_INFINITE );
			chThdSleepMilliseconds( 100 );
		}*/

		RawModbusClient* client = new RawModbusClient;
		client->setIODevice( usart );
		client->setFrameType( ModbusDevice::FrameType::Rtu );

		while( true )
		{
			uint16_t data[2];
			if( !client->readHoldingRegisters( 1, 0, 2, data ) )
				assert( false );
			assert( data[0] == 0xbad0 && data[1] == 0x4255 );
		}

		return 0;
	}
}