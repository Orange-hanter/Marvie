#include "MarvieDevice.h"

//uint8_t data[513] __attribute__( ( section( ".ram4" ) ) );

#include "Core/DateTimeService.h"
#include "Drivers/Network/Ethernet/EthernetThread.h"

//#include "Tests/EventTest.hpp"
//#include "Tests/FileSystemTest.hpp"
//#include "Tests/GsmPPPTest.hpp"
//#include "Tests/LwipEthernetTest.hpp"
//#include "Tests/MLinkTest.hpp"
//#include "Tests/MarvieLogSystemTest.hpp"
//#include "Tests/ModbusServerTest.hpp"
//#include "Tests/ModbusTest.hpp"
//#include "Tests/MultipleBRSensorsReaderTest.hpp"
//#include "Tests/PingTest.hpp"
//#include "Tests/PowerDownTest.hpp"
//#include "Tests/RemoteTerminalTest.hpp"
//#include "Tests/SdDataUploader.hpp"
//#include "Tests/Sim800AtTest.hpp"
//#include "Tests/SingleBRSensorReaderTest.hpp"
//#include "Tests/ThreadTest.hpp"
//#include "Tests/TimerTest.hpp"
//#include "Tests/UdpStressTestServer/UdpStressTestServer.h"
//#include "Tests/FirmwareTransferServiceTest.hpp"
//#include "Tests/UsbSduTest.hpp"

#include <algorithm>

extern char _bootloaderVersion[15 + 1];

int main()
{
	static_assert( __gthread_active_p() == 1, "__gthread_active_p() != 1" );
	static_assert( std::__default_lock_policy == std::_S_atomic, "__default_lock_policy != _S_atomic" );

	/*for( int i = 0x10000000; i < 0x10000000 + 64 * 1024; i += 4 )
		*( uint32_t* )i = 0;*/
	rccEnableAHB1( RCC_AHB1ENR_BKPSRAMEN, true );
	halInit();
	chSysInit();

	struct BootloaderMetaData
	{
		char version[15 + 1];
	}* metaData = reinterpret_cast< BootloaderMetaData* >( 0x10000000 + 42 * 1024 );
	metaData->version[15] = 0;
	strcpy( _bootloaderVersion, metaData->version );

	enum IWDGPrescaler
	{
		IWDGPrescaler4   = 0,
		IWDGPrescaler8   = 1,
		IWDGPrescaler16  = 2,
		IWDGPrescaler32  = 3,
		IWDGPrescaler64  = 4,
		IWDGPrescaler128 = 5,
		IWDGPrescaler256 = 6,
	};
	static WDGConfig wdgConfig;
	wdgConfig.pr = IWDGPrescaler256;
	wdgConfig.rlr = 0x0FFF;
	wdgStart( &WDGD1, &wdgConfig );
	wdgReset( &WDGD1 );

	ObjectMemoryUtilizer::instance()->runUtilizer( LOWPRIO );
	tcpip_init( nullptr, nullptr );

	/*SingleBRSensorReaderTest::test();
	MultipleBRSensorsReaderTest::test();
	while( true );*/

	/*TimerTest::test();
	while( true );*/

	/*EventTest::test();
	while( true );*/

	/*ThreadTest::test();
	while( true );*/

	/*FirmwareTransferServiceTest::test();
	while( true );*/

	/*Sim800Test::test();
	while(true);*/

	/*UsbSduTest::test();
	while( true );*/

	/*PPPServerTest::test();*/

	/*SdDataUploader::main();
	while( true );*/

	/*PowerDownTest::test();
	while( true );*/

	/*MarvieLogSystemTest::test();
	while( true );*/

	/*FileSystemTest::test();
	while( true );*/

	//  {
	//  	ObjectMemoryUtilizer::instance()->runUtilizer( LOWPRIO );
	//  	tcpip_init( nullptr, nullptr );

	//  	auto conf = EthernetThread::instance()->currentConfig();
	//  	conf.addressMode = EthernetThread::AddressMode::Static;
	//  	conf.ipAddress = IpAddress( 192, 168, 10, 10 );
	//  	conf.netmask = 0xFFFFFF00;
	//  	conf.gateway = IpAddress( 192, 168, 10, 1 );
	//  	EthernetThread::instance()->setConfig( conf );
	//  	EthernetThread::instance()->startThread();

	//  	chThdSleepMilliseconds( 3000 );
	//  	PingTest::test();
	//  	while( true )
	//  		;

	//  	//	/*thread_reference_t ref = nullptr;
	//  	//	chSysLock();
	//  	//	chThdSuspendS( &ref );
	//  	//	chSysUnlock();*/

	//  	//	MLinkTest::test();
	//  	//	while( true );
	//  }

	/*LwipEthernetTest::test();
	while( true );*/

	/*GsmPPPTest::test();
	while( true );*/

	{
		/*auto conf = EthernetThread::instance()->currentConfig();
		conf.addressMode = EthernetThread::AddressMode::Static;
		conf.ipAddress = IpAddress( 192, 168, 10, 10 );
		conf.netmask = 0xFFFFFF00;
		conf.gateway = IpAddress( 192, 168, 10, 1 );
		EthernetThread::instance()->setConfig( conf );
		EthernetThread::instance()->startThread();
		RemoteTerminalTest::test();
		while( true )
			;*/
	}

	MarvieDevice::instance()->exec();
	while( true )
		;

	/*auto conf = EthernetThread::instance()->currentConfig();
	conf.addressMode = EthernetThread::AddressMode::Static;
	conf.ipAddress = IpAddress( 192, 168, 10, 10 );
	conf.netmask = 0xFFFFFF00;
	conf.gateway = IpAddress( 192, 168, 10, 1 );
	EthernetThread::instance()->setConfig( conf );
	EthernetThread::instance()->startThread();*/

	/*palSetPadMode( GPIOA, 9, PAL_MODE_ALTERNATE( GPIO_AF_USART1 ) );
	palSetPadMode( GPIOA, 10, PAL_MODE_ALTERNATE( GPIO_AF_USART1 ) );
	Usart* terminal = Usart::instance( &SD1 );
	terminal->open( 115200 );
	terminal->write( ( uint8_t* )"Start ethernet...\r", 18, TIME_INFINITE );*/

	/*Concurrent::run( []() {
		UdpStressTestServer* server = new UdpStressTestServer( 1112 );
		server->exec();
	} );*/

	/*EvtListener listener;
 	EthernetThread::instance()->eventSource()->registerMask( &listener, EVENT_MASK( 0 ) );
 	while( true )
 	{
 		chEvtWaitAny( ALL_EVENTS );
 		eventflags_t flags = listener.getAndClearFlags();
 		if( flags & EthernetThread::Event::NetworkAddressChanged )
 		{
 			IpAddress ipAddr = EthernetThread::instance()->networkAddress();
 			char str[50];
 			sprintf( str, "%d.%d.%d.%d\n", ipAddr.addr[3], ipAddr.addr[2], ipAddr.addr[1], ipAddr.addr[0] );
 			terminal->write( ( uint8_t* )str, strlen( str ), TIME_INFINITE );
 		}
 		if( flags & EthernetThread::Event::LinkStatusChanged )
 		{
 			if( EthernetThread::instance()->linkStatus() == EthernetThread::LinkStatus::Up )
 				terminal->write( ( uint8_t* )"LinkUp\n", 7, TIME_INFINITE );
 			else
 				terminal->write( ( uint8_t* )"LinkDown\n", 9, TIME_INFINITE );
 		}
 	}*/

	/*Concurrent::run( []() {
		TcpServer* server = new TcpServer;
		server->listen( 42420 );
		while( server->isListening() )
		{
			if( server->waitForNewConnection( TIME_MS2I( 100 ) ) )
			{
				TcpSocket* socket = server->nextPendingConnection();
				Concurrent::run( ThreadProperties( 2048, NORMALPRIO ), [socket]() {
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
				} );
			}
		}
	} );*/

	/*guardArray = new uint8_t[1024 * 10];
	for( int i = 0; i < 1024 * 10; ++i )
		guardArray[i] = 0;*/

	//ModbusTest::test();
	//ModbusServerTest::test();
	/*gsmPPPTest();
	while( true );*/

	return 0;
}