#include "hal.h"
#include "lwipthread.h"

#include "Core/Concurrent.h"
#include "Core/ObjectMemoryUtilizer.h"

#include "Network/UdpSocket.h"
#include "Network/TcpSocket.h"
#include "Network/TcpServer.h"

#include <string.h>

uint8_t data[2577*2];

int lwipEthernetTest()
{
	ObjectMemoryUtilizer::instance()->runUtilizer( NORMALPRIO );

	lwipInit( nullptr );

	//chThdSleepMilliseconds( 6000 );
	/*TcpSocket* socket = new TcpSocket;
	while( true )
	{		
		bool res = socket->connect( IpAddress( 192, 168, 1, 12 ), 7 );
		socket->waitForReadAvailable( 5, TIME_INFINITE );
		socket->peek( 0, data, 5 );
		socket->disconnect();
		if( socket->read( data, 2577 * 2 ) != 0 )
			int gg = 42;
		//delete socket;
		socket->disconnect();
		chThdSleepMilliseconds( 1000 );
	}*/

	TcpServer* server = new TcpServer;
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
	}

	return 0;
}