#include "Drivers/Interfaces/Usart.h"
#include "Drivers/LogicOutput.h"

#include "Drivers/Network/SimGsm/SimGsmPppModem.h"

//
static uint8_t ob[1024] = {};
static uint8_t ib[1024] = {};
//static uint8_t buffer[128] = {};
//
//int gsmPPPTest()
//{
//	palSetPadMode( GPIOD, 5, PAL_MODE_ALTERNATE( GPIO_AF_USART2 ) );
//	palSetPadMode( GPIOD, 6, PAL_MODE_ALTERNATE( GPIO_AF_USART2 ) );
//	Usart* gsmUsart = Usart::instance( &SD2 );
//	gsmUsart->setInputBuffer( ib, sizeof( ib ) );
//	gsmUsart->setOutputBuffer( ob, sizeof( ob ) );
//	gsmUsart->open( 115200 );
//
//	palSetPadMode( GPIOD, 8, PAL_MODE_ALTERNATE( GPIO_AF_USART3 ) );
//	palSetPadMode( GPIOD, 9, PAL_MODE_ALTERNATE( GPIO_AF_USART3 ) );
//	Usart* terminalUsart = Usart::instance( &SD3 );
//	terminalUsart->open();
//
//	LogicOutput out;
//	//out.attach( IOPD7, true );
//	out.attach( IOPD7 );
//
//// 	chThdSleepMilliseconds( 200 );
//// 	out.on();
//
//	out.on();
//	chThdSleepMilliseconds( 800 );
//	out.off();
//	chThdSleepMilliseconds( 1500 );
//	out.on();
//
//	EvtListener gsmListener, terminalListener;
//	gsmUsart->eventSource()->registerOne( &gsmListener, 0 );
//	terminalUsart->eventSource()->registerOne( &terminalListener, 1 );
//	while( true )
//	{
//		eventmask_t em = chEvtWaitAny( ALL_EVENTS );
//		if( em & 1 ) // gsm
//		{
//			eventflags_t flags = gsmListener.getAndClearFlags();
//			if( flags & CHN_INPUT_AVAILABLE )
//			{
//				uint32_t size = gsmUsart->readAvailable();
//				while( size )
//				{
//					uint32_t part = size;
//					if( part > sizeof( buffer ) )
//						part = sizeof( buffer );
//					gsmUsart->read( buffer, part, TIME_INFINITE );
//					terminalUsart->write( buffer, part, TIME_INFINITE );
//					size -= part;
//				}
//			}
//		}
//		if( em & 2 ) // terminal
//		{
//			eventflags_t flags = terminalListener.getAndClearFlags();
//			if( flags & CHN_INPUT_AVAILABLE )
//			{
//				uint32_t size = terminalUsart->readAvailable();
//				while( size )
//				{
//					uint32_t part = size;
//					if( part > sizeof( buffer ) )
//						part = sizeof( buffer );
//					terminalUsart->read( buffer, part, TIME_INFINITE );
//					gsmUsart->write( buffer, part, TIME_INFINITE );
//					size -= part;
//				}
//			}
//		}
//	}
//
//
//	chThdSleepMilliseconds( 6000 );
//
//	gsmUsart->write( ( uint8_t* )"\r\nAT\r\n", 6, TIME_INFINITE );
//
//	thread_reference_t ref = nullptr;
//	chSysLock();
//	chThdSuspendS( &ref );
//
//	return 0;
//}

#include "Network/TcpServer.h"
#include "Tests/UdpStressTestServer/UdpStressTestServer.h"
#include "Core/Concurrent.h"

int gsmPPPTest()
{
	palSetPadMode( GPIOD, 5, PAL_MODE_ALTERNATE( GPIO_AF_USART2 ) );
	palSetPadMode( GPIOD, 6, PAL_MODE_ALTERNATE( GPIO_AF_USART2 ) );
	Usart* gsmUsart = Usart::instance( &SD2 );
	gsmUsart->setInputBuffer( ib, sizeof( ib ) );
	gsmUsart->setOutputBuffer( ob, sizeof( ob ) );
	gsmUsart->open( 230400 );

	LogicOutput pwrKey;
	pwrKey.attach( IOPD1 );
	pwrKey.on();

	SimGsmPppModem* modem = new SimGsmPppModem( IOPD0, false, 0 );
	modem->setUsart( gsmUsart );
	modem->setApn( "m2m30.velcom.by" );
	modem->startModem();
	modem->waitForStateChange();
	modem->setAsDefault();

	Concurrent::_run( []()
	{
		UdpStressTestServer* server = new UdpStressTestServer( 1114 );
		server->exec();
	} );

	/*TcpServer* server = new TcpServer;
	server->listen( 42420 );
	while( server->isListening() )
	{
		if( server->waitForNewConnection( TIME_MS2I( 100 ) ) )
		{
			TcpSocket* socket = server->nextPendingConnection();
			Concurrent::run( [socket]()
			{
				uint8_t* data = new uint8_t[2048];
				while( true )
				{
					//chThdSleepMilliseconds( 3000 );
					if( socket->waitForReadAvailable( 1, TIME_MS2I( 100 ) ) )
					{
						while( socket->readAvailable() )
						{
							uint32_t len = socket->read( data, 2048, TIME_IMMEDIATE );
							if( socket->write( data, len ) == 0 )
								goto End;
						}
					}
					if( !socket->isOpen() )
						break;
				}
End:
				delete data;
				delete socket;
			}, 2048, NORMALPRIO );
		}
	}*/

	/*chThdSleepMilliseconds( 5000 );
	modem->stopModem();
	modem->waitForStatusChange();*/

	thread_reference_t ref = nullptr;
	chSysLock();
	chThdSuspendS( &ref );

	return 0;
}

/*
ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
//
//#include <string.h>
//#include "ch.h"
//#include "hal.h"
//#include "lwip/api.h"
//#include "chprintf.h"
//#include "lwip/tcpip.h"
//extern "C"
//{
//#include "netif/ppp/pppos.h"
//}
//
///* For logging and modem communication*/
//#define CRLF       "\r\n"
//#define MODEM_SD     SD2
//#define TRACE_SD     SD3
//
///* Lwip stack global variables*/
//static mutex_t trace_mutex;
//static ppp_pcb * g_ppp;
//static struct netif g_pppos_netif;
//
///* Prototypes*/
//static void logger_init( void );
//static void trace( const char* format, ... );
//
//static void lwip_ipstack_init( void );
//static void ppp_link_status_callback( ppp_pcb *pcb, int err_code, void *ctx );
//static u32_t ppp_output_callback( ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx );
//static void netif_status_callback( struct netif *nif );
//static void tcpip_init_done_callback( void *arg );
//static void init_pppos_netif( void );
//static void init_ok_callback( void );
//static void sleep( uint8_t second );
//
///*
//* PPP input thread
//*/
//static THD_WORKING_AREA( waPppInputThread, 1024 );
//
//static THD_FUNCTION( PppInputThread, arg ) {
//	( void )arg;
//
//	chRegSetThreadName( "pppos_rx_thread" );
//	u32_t len;
//	u8_t buffer[256];
//
//	if( g_ppp ) {
//		trace( "lwip (step 3.1/4): pppos_rx_thread stated" CRLF );
//
//		/* Please read the "PPPoS input path" chapter in the PPP documentation. */
//		while( true ) {
//			len = chnReadTimeout( &MODEM_SD, buffer, sizeof( buffer ), TIME_MS2I( 1000 ) );
//			if( len > 0 ) {
//				/* Pass received raw characters from PPPoS to be decoded through lwIP
//				* TCPIP thread using the TCPIP API. This is thread safe in all cases
//				* but you should avoid passing data byte after byte. */
//				pppos_input_tcpip( g_ppp, buffer, len );
//			}
//		}
//	}
//	else {
//		trace( "lwip error (step 3.1/4): ppp_pcb need initialization" CRLF );
//	}
//}
//
///*
//* Testing tcp client. Change the destination to a public IP address if IP_FORWARD is enabled on PPPD host
//*/
//static THD_WORKING_AREA( waTcpEchoThread, 1024 );
//
//#define BUFLEN  200
//#define MQTT_IP "192.168.141.227"
//
//static THD_FUNCTION( TcpEchoThread, arg ) {
//	( void )arg;
//
//	trace( "enter echo thread" CRLF );
//	sleep( 5 );
//
//	chRegSetThreadName( "tcpecho_thread" );
//
//	ip_addr_t target_ip;
//	IP4_ADDR( &target_ip, 192, 168, 141, 227 );
//
//	char buf[] = "my payload from build (" __FILE__ " on " __DATE__ ":" __TIME__ ")";
//
//	while( true ) {
//		trace( "mqtt: [V4]try connect to " MQTT_IP ":4000..." CRLF );
//		struct netconn* conn = netconn_new( NETCONN_TCP );
//
//		if( conn ) {
//			trace( "netconn_connect...." CRLF );
//			sleep( 5 );
//			err_t err = netconn_connect( conn, &target_ip, 4000 );
//			if( err == ERR_OK ) {
//				trace( "mqtt: connected to " MQTT_IP ":4000" CRLF );
//
//				while( netconn_write( conn, buf, strlen( buf ), NETCONN_COPY ) == ERR_OK ) {
//					trace( "mqtt: netconn_write ok for mqtt-connect with len: %d" CRLF, strlen( buf ) );
//					chThdSleepMilliseconds( 100 );
//				}
//
//				trace( "mqtt-error: mqtt-connect with len: %d" CRLF, strlen( buf ) );
//
//				/* Close connection and discard connection identifier. */
//				netconn_close( conn );
//			}
//			else {
//				trace( "mqtt-error: cannot connect to " MQTT_IP ":4000, sleep 5 seconds, error: %d" CRLF, err );
//				sleep( 5 );
//			}
//			netconn_delete( conn );
//			sleep( 1 );
//		}
//		else {
//			trace( "mqtt-error: cannot create netconn, sleep 5 seconds" CRLF );
//			sleep( 1 );
//		}
//	}
//}
//
///*
//* Application entry point.
//*/
//int gsmPPPTest( void ) {
//	
//	/* Initializing logger*/
//	logger_init();
//
//	/* Start ip stack*/
//	lwip_ipstack_init();
//
//	/*
//	* Normal main() thread activity, in this demo it does nothing except
//	* sleeping in a loop and check the button state.
//	*/
//	while( true ) {
//		sleep( 10 );
//	}
//}
//
///* Implementations*/
//static void logger_init( void ) {
//	chMtxObjectInit( &trace_mutex ); /* Initializing the mutex sharing the resource.*/
//
//	SerialConfig trace_sd_config = { 115200, 0, USART_CR2_STOP1_BITS | USART_CR2_LINEN, 0 };
//	sdStart( &TRACE_SD, &trace_sd_config ); /* TRACE_SD for debug, and console print*/
//	palSetPadMode( GPIOD, 8, PAL_MODE_ALTERNATE( 7 ) );
//	palSetPadMode( GPIOD, 9, PAL_MODE_ALTERNATE( 7 ) );
//}
//
//static void trace( const char* format, ... ) {
//	va_list ap;
//	va_start( ap, format );
//	chMtxLock( &trace_mutex );
//	chvprintf( ( BaseSequentialStream* )& TRACE_SD, format, ap );
//	chMtxUnlock( &trace_mutex );
//	va_end( ap );
//}
//
//static void sleep( uint8_t second ) {
//	chThdSleepMilliseconds( second * 1000 );
//}
//
//void timerCallback( void* p )
//{
//	chSysLockFromISR();
//	chEvtSignalI( reinterpret_cast< thread_t* >( p ), EVENT_MASK( 1 ) ); // InnerEvent
//	chSysUnlockFromISR();
//}
//
//#define SENSOR_TERMINATE_TEST_INTERVAL TIME_MS2I( 10000 )
//ByteRingIterator waitForResponse( IODevice* io, const char* response, uint32_t responseLen, sysinterval_t timeout )
//{
//	uint32_t pos = 0;
//	ByteRingIterator i, respBegin;
//	i = respBegin = io->inputBuffer()->begin();
//
//	enum Event : eventmask_t { IODeviceEvent = EVENT_MASK( 0 ), InnerEvent = EVENT_MASK( 1 ) };
//	chEvtGetAndClearEvents( IODeviceEvent | InnerEvent );
//
//	virtual_timer_t timer;
//	chVTObjectInit( &timer );
//	sysinterval_t nextInterval = timeout;
//	if( nextInterval > SENSOR_TERMINATE_TEST_INTERVAL )
//		nextInterval = SENSOR_TERMINATE_TEST_INTERVAL;
//	chVTSet( &timer, nextInterval, timerCallback, chThdGetSelfX() );
//
//	EvtListener ioDeviceListener;
//	io->eventSource()->registerMaskWithFlags( &ioDeviceListener, IODeviceEvent, CHN_INPUT_AVAILABLE );
//	while( true )
//	{
//		ByteRingIterator end = io->inputBuffer()->end();
//		while( i != end )
//		{
//			if( *i == response[pos++] )
//			{
//				if( pos == responseLen )
//					goto Leave;
//				++i;
//			}
//			else
//			{
//				pos = 0;
//				i = ++respBegin;
//			}
//		}
//		eventmask_t em = chEvtWaitAny( IODeviceEvent | InnerEvent );
//		if( em & InnerEvent )
//		{
//			timeout -= nextInterval;
//			if( timeout == 0 || chThdShouldTerminateX() )
//				break;
//			if( timeout > SENSOR_TERMINATE_TEST_INTERVAL )
//				nextInterval = SENSOR_TERMINATE_TEST_INTERVAL;
//			else
//				nextInterval = timeout;
//			chVTSet( &timer, nextInterval, timerCallback, chThdGetSelfX() );
//		}
//		if( em & IODeviceEvent )
//			ioDeviceListener.getAndClearFlags();
//	}
//
//Leave:
//	io->eventSource()->unregister( &ioDeviceListener );
//	chVTReset( &timer );
//
//	// chEvtGetAndClearEvents( IODeviceEvent | InnerEvent );
//	if( pos == responseLen )
//		return respBegin;
//	return ByteRingIterator();
//}
//
//static void lwip_ipstack_init( void ) {
//
//	trace( "lwip (step 0/4): init usart port on MODEM_SD" CRLF );
//
//	Usart* gsmUsart = Usart::instance( &MODEM_SD );
//	gsmUsart->setInputBuffer( ib, sizeof( ib ) );
//	gsmUsart->setOutputBuffer( ob, sizeof( ob ) );
//	gsmUsart->open( 115200 );
//	palSetPadMode( GPIOD, 5, PAL_MODE_ALTERNATE( GPIO_AF_USART2 ) );
//	palSetPadMode( GPIOD, 6, PAL_MODE_ALTERNATE( GPIO_AF_USART2 ) );
//
//	LogicOutput out;
//	out.attach( IOPD7 );
//
//	out.on();
//	chThdSleepMilliseconds( 800 );
//	out.off();
//	chThdSleepMilliseconds( 1500 );
//	out.on();
//
//	while( true )
//	{
//		out.on();
//		chThdSleepMilliseconds( 800 );
//		out.off();
//		chThdSleepMilliseconds( 1500 );
//		out.on();
//
//		/*if( !waitForResponse( gsmUsart, "+CPIN: SIM PIN\r\n", 16, TIME_MS2I( 6000 ) ).isValid() )
//		{
//			trace( "PIN timeout" CRLF );
//			continue;
//		}*/
//		//gsmUsart->read( nullptr, gsmUsart->readAvailable(), TIME_IMMEDIATE );
//		//gsmUsart->write( ( uint8_t* )"at+cpin=\"1111\"\r\n", 16, TIME_INFINITE );
//		if( !waitForResponse( gsmUsart, "SMS Ready\r\n", 11, TIME_MS2I( 6000 ) ).isValid() )
//		{
//			trace( "Ready timeout" CRLF );
//			continue;
//		}
//		gsmUsart->read( nullptr, gsmUsart->readAvailable(), TIME_IMMEDIATE );
//		gsmUsart->write( ( uint8_t* )"at+cgdcont=1,\"IP\",\"m2m30.velcom.by\"\r\n", 37, TIME_INFINITE );
//		if( !waitForResponse( gsmUsart, "OK\r\n", 4, TIME_MS2I( 6000 ) ).isValid() )
//		{
//			trace( "APN timeout" CRLF );
//			continue;
//		}
//		gsmUsart->read( nullptr, gsmUsart->readAvailable(), TIME_IMMEDIATE );
//		gsmUsart->write( ( uint8_t* )"ATD*99#\r\n", 9, TIME_INFINITE );
//		if( !waitForResponse( gsmUsart, "CONNECT\r\n", 9, TIME_MS2I( 6000 ) ).isValid() )
//		{
//			trace( "PPP timeout" CRLF );
//			continue;
//		}
//		gsmUsart->read( nullptr, gsmUsart->readAvailable(), TIME_IMMEDIATE );
//		break;
//	}
//	
//	trace( "lwip (step 1/4): start initialization flow. (build %s-%s)" CRLF, __DATE__, __TIME__ );
//
//	LOCK_TCPIP_CORE();
//	tcpip_init_done_callback(nullptr);
//	UNLOCK_TCPIP_CORE();
//}
//
//static void tcpip_init_done_callback( void *arg ) {
//	LWIP_UNUSED_ARG( arg );
//
//	trace( "lwip (step 2/4): TCP/IP initialized, now start netifs" CRLF );
//	init_pppos_netif();
//
//	/* Start PPPoS receiver thread */
//	trace( "lwip (step 3.1/4): TCP/IP initialized, now start PPPoS receiver thread" CRLF );
//	chThdCreateStatic( waPppInputThread, sizeof( waPppInputThread ), NORMALPRIO - 1, PppInputThread, NULL );
//}
//
//static void init_pppos_netif( void ) {
//	g_ppp = pppos_create( &g_pppos_netif, ppp_output_callback, ppp_link_status_callback, NULL );
//	if( g_ppp ) {
//		trace( "lwip (step 3.2/4): request pppos_create successful, start connect" CRLF );
//		ppp_connect( g_ppp, 0 );
//		netif_set_status_callback( &g_pppos_netif, netif_status_callback );
//		netif_set_default( &g_pppos_netif );
//	}
//	else {
//		trace( "lwip error (step 3.2/4): request pppos_create failed" CRLF );
//	}
//}
//
//static void ppp_link_status_callback( ppp_pcb *pcb, int err_code, void *ctx ) {
//	LWIP_UNUSED_ARG( pcb );
//	LWIP_UNUSED_ARG( ctx );
//
//	if( err_code == PPPERR_NONE ) /* No error. */ {
//		trace( "lwip (step 4.2/4): ppp_link_status_cb: PPPERR_NONE" CRLF );
//		trace( "lwip (step 4.3/4): start custom applications" CRLF );
//		init_ok_callback();
//	}
//	else {
//		trace( "lwip error (step 4.2/4): ppp_link_status_cb: %d" CRLF, err_code );
//	}
//}
//
//static u32_t ppp_output_callback( ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx ) {
//	LWIP_UNUSED_ARG( pcb );
//	LWIP_UNUSED_ARG( ctx );
//
//	u32_t sended = 0;
//	while( sended < len ) {
//		sended += chnWrite( &MODEM_SD, data + sended, len - sended );
//	}
//	return sended;
//}
//
//static void netif_status_callback( struct netif *nif ) {
//	trace( "lwip (step 4.1/4): netif %c%c%d: %s" CRLF,
//		   nif->name[0], nif->name[1], nif->num, netif_is_up( nif ) ? "UP" : "DOWN" );
//	trace( "IPV4: Host at %s ", ip4addr_ntoa( netif_ip4_addr( nif ) ) );
//	trace( "mask %s ", ip4addr_ntoa( netif_ip4_netmask( nif ) ) );
//	trace( "gateway %s" CRLF, ip4addr_ntoa( netif_ip4_gw( nif ) ) );
//}
//
//static void init_ok_callback( void ) {
//	trace( "ready to start application " CRLF );
//	chThdCreateStatic( waTcpEchoThread, sizeof( waTcpEchoThread ), NORMALPRIO - 2, TcpEchoThread, NULL );
//}