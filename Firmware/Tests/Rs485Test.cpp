#include "Drivers/Interfaces/Rs485.h"
#include "Drivers/Interfaces/SharedRs485.h"

static uint8_t data[10];

int main()
{
	halInit();
	chSysInit();

	palSetPadMode( GPIOD, 8, PAL_MODE_ALTERNATE( GPIO_AF_USART3 ) | PAL_STM32_OSPEED_HIGHEST );
	palSetPadMode( GPIOD, 9, PAL_MODE_ALTERNATE( GPIO_AF_USART3 ) | PAL_STM32_OSPEED_HIGHEST );
	Usart* usart = Usart::instance( &SD3 );
	usart->open();

	SharedRs485Control* control = new SharedRs485Control( usart );
	SharedRs485* sharedRs485[2] = { control->create( IOPB15, IOPB14 ), control->create( IOPD11, IOPD10 ) };

	sharedRs485[0]->write( ( const uint8_t* )"Hello", 5 );
	sharedRs485[1]->write( ( const uint8_t* )"Hello", 5 );

	sharedRs485[1]->read( data, 5 );
	sharedRs485[0]->enable();
	sharedRs485[0]->read( data + 5, 5 );

	while( true )
		;	
}