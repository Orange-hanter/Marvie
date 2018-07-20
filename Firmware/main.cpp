#include "MarvieDevice.h"

//uint8_t data[513] __attribute__( ( section( ".ram4" ) ) );

#include "Lib/sha1/sha1.h"

#include "Drivers/Interfaces/Rs485.h"

int main()
{
	halInit();
	chSysInit();

	/*palSetPadMode( GPIOC, 8, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
	palSetPadMode( GPIOC, 12, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
	palSetPadMode( GPIOD, 2, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
	SDCConfig sdcConfig;
	sdcConfig.scratchpad = nullptr;
	sdcConfig.bus_width = SDC_MODE_1BIT;
	sdcStart( &SDCD1, &sdcConfig );

	auto gg = sdcConnect( &SDCD1 );

	chThdSleepSeconds( 3 );
	FATFS fatFs;
	auto err = f_mount( &fatFs, "/", 1 );
	while( true );*/

	//volatile bool sdOk = ( sdcConnect( &SDCD1 ) == HAL_SUCCESS );


	/*palSetPadMode( GPIOC, 7, PAL_MODE_ALTERNATE( GPIO_AF_USART6 ) );
	palSetPadMode( GPIOC, 6, PAL_MODE_ALTERNATE( GPIO_AF_USART6 ) );
	Usart* usart = Usart::instance( &SD6 );
	usart->open();
	Rs485* rs485 = new Rs485( usart, IOPB15, IOPB14 );
	rs485->write( ( const uint8_t* )"Hello", 5 );
	//rs485->disable();
	while( true )
	{

	}*/
	//&SD3, IOPD9, IOPD8/*IOPB10*/, IOPB15, IOPB14


	
	/*Usart* usart = Usart::instance( &SD2 );
	//usart->setOutputBuffer( ob, 1024 );
	//usart->setInputBuffer( ib, 2048 );
	usart->open();
	palSetPadMode( GPIOD, 5, PAL_MODE_ALTERNATE( GPIO_AF_USART2 ) );
	palSetPadMode( GPIOD, 6, PAL_MODE_ALTERNATE( GPIO_AF_USART2 ) );

	while( true )
		usart->write( ( const uint8_t* )"Hello", 5, TIME_INFINITE );*/


	ObjectMemoryUtilizer::instance()->runUtilizer( LOWPRIO );

	////void* p = CCMemoryHeap::alloc( 8 );

	////void ggg;

	//uint32_t a = 42;
	//Concurrent::run( [&a]()
	//{
	//	/*while( true )
	//	{
	//		int bb = 5;
	//	}*/

	//	//return 0;
	//}, 512, NORMALPRIO );

	//thread_reference_t ref = nullptr;
	//chSysLock();
	//chThdSuspendS( &ref );
	
	MarvieDevice::instance()->exec();

	return 0;
}