#include "Drivers/Interfaces/Usart.h"
#include <string.h>

#define SIM800L

namespace Sim800Test
{
	// &SD2, IOPD6, IOPD5

	void test( void )
	{
		uint8_t buf[256] = {};
		palSetPadMode( GPIOD, 5, PAL_MODE_ALTERNATE( GPIO_AF_USART2 ) );
		palSetPadMode( GPIOD, 6, PAL_MODE_ALTERNATE( GPIO_AF_USART2 ) );
		palSetPadMode( GPIOD, 0, PAL_MODE_OUTPUT_PUSHPULL );
		palSetPadMode( GPIOD, 1, PAL_MODE_OUTPUT_PUSHPULL );
		
#ifdef SIM800C
		palSetPad( GPIOD, 1 );
#else
		palClearPad( GPIOD, 1 );
#endif // SIM800C

		palClearPad( GPIOD, 0 );
		chThdSleepMilliseconds( 100 );
		palSetPad( GPIOD, 0 );
		chThdSleepMilliseconds( 200 );
		palClearPad( GPIOD, 0 );
	
		

		Usart* usart = Usart::instance( &SD2 );
		usart->setInputBuffer( buf, sizeof( buf ) );
		usart->open( 115200 );

		chThdSleepMilliseconds( 4000 );
		usart->write( ( uint8_t* )"AT\n\r", 4, TIME_INFINITE );
        chThdSleepMilliseconds( 100 );
		usart->write( ( uint8_t* )"AT\n\r", 4, TIME_INFINITE );
        chThdSleepMilliseconds( 100 );
		usart->write( ( uint8_t* )"AT\n\r", 4, TIME_INFINITE );
        chThdSleepMilliseconds( 100 );
		usart->write( ( uint8_t* )"AT\n\r", 4, TIME_INFINITE );
        chThdSleepMilliseconds( 100 );
		usart->write( ( uint8_t* )"AT+IPR=230400\n\r", 15, TIME_INFINITE );
        chThdSleepMilliseconds( 300 );
		usart->setBaudRate( 230400 );
		usart->write( ( uint8_t* )"ATE0\n\r", 6, TIME_INFINITE );
        chThdSleepMilliseconds( 300 );
		usart->write( ( uint8_t* )"AT+GMM\n\r", 8, TIME_INFINITE );
		chThdSleepMilliseconds( 300 );
	usart->write( ( uint8_t* )"AT&W\n\r", 6, TIME_INFINITE );
        chThdSleepMilliseconds( 300 );

	palSetPad( GPIOD, 0 );

	}
} // namespace Sim800Test