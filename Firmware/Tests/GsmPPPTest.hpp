#include "Drivers/Interfaces/Usart.h"
#include "Drivers/LogicOutput.h"

static uint8_t ob[1024] = {};
static uint8_t ib[1024] = {};
static uint8_t buffer[128] = {};

int gsmPPPTest()
{
	palSetPadMode( GPIOD, 5, PAL_MODE_ALTERNATE( GPIO_AF_USART2 ) );
	palSetPadMode( GPIOD, 6, PAL_MODE_ALTERNATE( GPIO_AF_USART2 ) );
	Usart* gsmUsart = Usart::instance( &SD2 );
	gsmUsart->setInputBuffer( ib, sizeof( ib ) );
	gsmUsart->setOutputBuffer( ob, sizeof( ob ) );
	gsmUsart->open( 9600 );

	palSetPadMode( GPIOD, 8, PAL_MODE_ALTERNATE( GPIO_AF_USART3 ) );
	palSetPadMode( GPIOD, 9, PAL_MODE_ALTERNATE( GPIO_AF_USART3 ) );
	Usart* terminalUsart = Usart::instance( &SD3 );
	terminalUsart->open();

	LogicOutput out;
	out.attach( IOPD7, true );

	chThdSleepMilliseconds( 200 );
	out.on();

	EvtListener gsmListener, terminalListener;
	gsmUsart->eventSource()->registerOne( &gsmListener, 0 );
	terminalUsart->eventSource()->registerOne( &terminalListener, 1 );
	while( true )
	{
		eventmask_t em = chEvtWaitAny( ALL_EVENTS );
		if( em & 1 ) // gsm
		{
			eventflags_t flags = gsmListener.getAndClearFlags();
			if( flags & CHN_INPUT_AVAILABLE )
			{
				uint32_t size = gsmUsart->readAvailable();
				while( size )
				{
					uint32_t part = size;
					if( part > sizeof( buffer ) )
						part = sizeof( buffer );
					gsmUsart->read( buffer, part, TIME_INFINITE );
					terminalUsart->write( buffer, part, TIME_INFINITE );
					size -= part;
				}
			}
		}
		if( em & 2 ) // terminal
		{
			eventflags_t flags = terminalListener.getAndClearFlags();
			if( flags & CHN_INPUT_AVAILABLE )
			{
				uint32_t size = terminalUsart->readAvailable();
				while( size )
				{
					uint32_t part = size;
					if( part > sizeof( buffer ) )
						part = sizeof( buffer );
					terminalUsart->read( buffer, part, TIME_INFINITE );
					gsmUsart->write( buffer, part, TIME_INFINITE );
					size -= part;
				}
			}
		}
	}


	chThdSleepMilliseconds( 6000 );

	gsmUsart->write( ( uint8_t* )"\r\nAT\r\n", 6, TIME_INFINITE );

	thread_reference_t ref = nullptr;
	chSysLock();
	chThdSuspendS( &ref );

	return 0;
}