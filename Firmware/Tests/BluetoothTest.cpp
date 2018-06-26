#include "hal.h"
#include "Drivers/LogicOutput.h"

#include "Drivers/Interfaces/Usart.h"
#include <string.h>

uint8_t ob[128] = {};
uint8_t ib[128] = {};

int main()
{
	halInit();
	chSysInit();

	/*LogicOutput o;
	o.attach( IOPC2 );
	o.on();

	while( true )
		;*/

	Usart* usart = Usart::instance( &SD4 );
	usart->setOutputBuffer( ob, 128 );
	usart->setInputBuffer( ib, 128 );
	usart->open( 115200 );
	palSetPadMode( GPIOA, 0, PAL_MODE_ALTERNATE( GPIO_AF_UART4 ) );
	palSetPadMode( GPIOA, 1, PAL_MODE_ALTERNATE( GPIO_AF_UART4 ) );

	uint8_t data[20];
	while( true  )
	{
		usart->waitForReadAvailable( sizeof( "fuck you\r\n" ) - 1, TIME_INFINITE );
		usart->read( data, sizeof( "fuck you\r\n" ) - 1, TIME_INFINITE );
		if( strcmp( ( char* )data, "fuck you\r\n" ) == 0 )
			usart->write( ( uint8_t* )"no, fuck you\r\n", 14, TIME_INFINITE );
	}

	/*while( true )
	{
		usart->write( ( uint8_t* )"AT+PIN\r\n", 8, TIME_INFINITE );
		chThdSleep( TIME_S2I( 1 ) );
	}*/

	return 0;
}