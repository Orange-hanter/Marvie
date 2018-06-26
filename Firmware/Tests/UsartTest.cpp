#include "ch.h"
#include "hal.h"
#include "rt_test_root.h"
#include "oslib_test_root.h"

#include "Drivers/Interfaces/Usart.h"
#include <string.h>

uint8_t ob[3] = {};
uint8_t ib[5] = {};
char tmp[100] = {};

class WriteThread : private BaseStaticThread< 128 >
{
public:
	void start()
	{
		BaseStaticThread::start( NORMALPRIO );
	}

private:
	void main() final override
	{
		Usart* usart = Usart::instance( &SD3 );
		while( true )
		{
			usart->write( ( uint8_t* )"Hello\n", 6, TIME_INFINITE );
			chThdSleepSeconds( 1 );
		}
	}
} writeThread;

int main( void )
{
	halInit();
	chSysInit();

	Usart* usart = Usart::instance( &SD3 );
	usart->setOutputBuffer( ob, 3 );
	usart->setInputBuffer( ib, 5 );
	usart->open( 38400 );
	palSetPadMode( GPIOB, 10, PAL_MODE_ALTERNATE( GPIO_AF_USART3 ) );
	palSetPadMode( GPIOB, 11, PAL_MODE_ALTERNATE( GPIO_AF_USART3 ) );

	writeThread.start();

	EvtListener listener;
	usart->eventSource()->registerMaskWithFlags( &listener, EVENT_MASK( 0 ), CHN_INPUT_AVAILABLE );
	uint8_t* p = ( uint8_t* )tmp;
	while( true )
	{
		eventmask_t em = chEvtWaitAny( EVENT_MASK( 0 ) );
		if( em & EVENT_MASK( 0 ) )
		{
			listener.getAndClearFlags();
			//chThdSleepMilliseconds( 500 );
			auto begin = usart->inputBuffer()->begin();
			auto end = usart->inputBuffer()->end();			
			for( auto i = begin; i != end; ++i )
				*( p++ ) = *i;
			usart->inputBuffer()->read( nullptr, end - begin );
		}
	}
}