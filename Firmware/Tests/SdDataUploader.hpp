#pragma once

#include "Network/TcpServer.h"
#include "Drivers/Network//Ethernet/EthernetThread.h"

namespace SdDataUploader
{
	int main()
	{
		palSetPadMode( GPIOC, 8, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );    // D0
		//palSetPadMode( GPIOC, 9, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );  // D1
		//palSetPadMode( GPIOC, 10, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST ); // D2
		palSetPadMode( GPIOC, 11, PAL_MODE_INPUT | PAL_STM32_OSPEED_HIGHEST );               // D3/CD
		palSetPadMode( GPIOC, 12, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );   // CLK
		palSetPadMode( GPIOD, 2, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );    // CMD
		static SDCConfig sdcConfig;
		sdcConfig.scratchpad = nullptr;
		sdcConfig.bus_width = SDC_MODE_1BIT;
		sdcStart( &SDCD1, &sdcConfig/*nullptr*/ );
		if( !sdcConnect( &SDCD1 ) )
		{
			sdcStop( &SDCD1 );
			return 1;
		}

		uint8_t* buffer = new uint8_t[512];

		auto ethConf = EthernetThread::instance()->currentConfig();
		ethConf.addressMode = EthernetThread::AddressMode::Static;
		ethConf.ipAddress = IpAddress( 192, 168, 2, 10 );
		ethConf.netmask = 0xFFFFFF00;
		ethConf.gateway = IpAddress( 192, 168, 2, 1 );
		EthernetThread::instance()->setConfig( ethConf );
		EthernetThread::instance()->startThread( NORMALPRIO );

		TcpServer server;
		server.listen( 55555 );
		while( true )
		{
			server.waitForNewConnection();
			auto socket = server.nextPendingConnection();
			if( !socket->waitForReadAvailable( 8, TIME_MS2I( 5000 ) ) )
				delete socket;
			struct Request
			{
				uint32_t beginSector;
				uint32_t endSector;
			} req;
			socket->read( ( uint8_t* )&req, sizeof( req ) );
			for( auto sector = req.beginSector; sector != req.endSector; ++sector )
			{
				if( sdcRead( &SDCD1, sector, buffer, 1 ) != HAL_SUCCESS )
					break;
				if( socket->write( buffer, 512 ) != 512 )
					break;
			}
			socket->close();
			delete socket;
		}

		delete buffer;
		EthernetThread::instance()->stopThread();
		EthernetThread::instance()->waitForStop();
		sdcDisconnect( &SDCD1 );
		sdcStop( &SDCD1 );

		return 0;
	}
}