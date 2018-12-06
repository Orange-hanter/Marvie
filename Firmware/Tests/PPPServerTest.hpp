#pragma once

#include "lwip/api.h"
#include "lwip/tcpip.h"
extern "C"
{
#include "netif/ppp/pppos.h"
}
#include <string.h>
#include "Core/Assert.h"
#include "Drivers/Interfaces/Usart.h"

namespace PPPServerTest
{
	class Test
	{
	public:
		enum { IOEvent = 2, TimerEvent = 4, LinkDownEvent = 8, LinkUpEvent = 16 };
		netif pppNetif;
		ppp_pcb* pppPcb;
		uint32_t len;
		uint8_t buffer[256];
		virtual_timer_t timer;
		Usart* usart;
		thread_t* thread_ref;
		uint8_t ob[4096] = {}, ib[4096] = {};

		void exec()
		{
			thread_ref = chThdGetSelfX();

			memset( &pppNetif, 0, sizeof( pppNetif ) );
			memset( buffer, 0, sizeof( buffer ) );
			LOCK_TCPIP_CORE();
			pppPcb = pppos_create( &pppNetif, outputCallback, linkStatusCallback, this );
			netif_set_status_callback( &pppNetif, netifStatusCallback );
			netif_get_client_data( &pppNetif, LWIP_NETIF_CLIENT_DATA_INDEX_MAX ) = this;
			ip4_addr ip, netmask, gateway;
			ip.addr = LWIP_MAKEU32( 10, 10, 10, 1 );
			netmask.addr = LWIP_MAKEU32( 255, 255, 255, 0 );
			gateway.addr = LWIP_MAKEU32( 10, 10, 10, 10 );
			netif_set_addr( &pppNetif, &ip, &netmask, &gateway );
			UNLOCK_TCPIP_CORE();
			len = 0;
			chVTObjectInit( &timer );

			//&SD1, IOPA10, IOPA9
			palSetPadMode( GPIOA, 10, PAL_MODE_ALTERNATE( GPIO_AF_USART1 ) );
			palSetPadMode( GPIOA, 9, PAL_MODE_ALTERNATE( GPIO_AF_USART1 ) );
			usart = Usart::instance( &SD1 );
			usart->setInputBuffer( ib, sizeof( ib ) );
			usart->setOutputBuffer( ob, sizeof( ob ) );
			usart->open( 115200 );

			//while( true )
			//usart->write( ( uint8_t* )"Hello gg", 8, TIME_INFINITE );

			LOCK_TCPIP_CORE();
			err_t err = ppp_listen( pppPcb );
			UNLOCK_TCPIP_CORE();
			if( err != ERR_OK )
				assert( false );

			EvtListener ioListener;

#define INPUT_DELAY TIME_MS2I( 1 )
			usart->eventSource()->registerMaskWithFlags( &ioListener, IOEvent, CHN_INPUT_AVAILABLE );
			//chVTSet( &timer, INPUT_DELAY, timerCallback, this );

			while( true )
			{
				eventmask_t em = chEvtWaitAny( ALL_EVENTS );
				if( em & LinkDownEvent )
					assert( false )
				if( em & LinkUpEvent )
				{
					int gg = 42;
				}
				if( em & IOEvent )
				{
					while( true )
					{
						len += usart->read( buffer + len, sizeof( buffer ) - len, TIME_IMMEDIATE );
						if( len == sizeof( buffer ) )
						{
							pppos_input_tcpip( pppPcb, buffer, sizeof( buffer ) );
							len = 0;
						}
						else
							break;
					}
					chSysLock();
					chVTSetI( &timer, INPUT_DELAY, timerCallback, this );
					chEvtGetAndClearEventsI( TimerEvent );
					chSysUnlock();
					continue;
				}
				if( em & TimerEvent )
				{
					if( len )
					{
						pppos_input_tcpip( pppPcb, buffer, len );
						len = 0;
					}
				}
			}
		}

		static u32_t outputCallback( ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx )
		{
			if( ctx == nullptr )
				return 0;
			Test* t = ( Test* )ctx;
			return t->usart->write( data, len, TIME_IMMEDIATE );
		}
		static void linkStatusCallback( ppp_pcb *pcb, int err_code, void *ctx )
		{
			Test* t = ( Test* )ctx;
			if( err_code == PPPERR_NONE )
				chEvtSignal( t->thread_ref, LinkUpEvent );
			else
				chEvtSignal( t->thread_ref, LinkDownEvent );
		}
		static void netifStatusCallback( netif *nif )
		{

		}
		static void timerCallback( void* p )
		{
			Test* t = ( Test* )p;
			chSysLockFromISR();
			chEvtSignalI( t->thread_ref, TimerEvent );
			chSysUnlockFromISR();
		}
	};

	int test()
	{
		Test* test = new Test;
		test->exec();

		return 0;
	}
}