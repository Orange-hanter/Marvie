#include "Drivers/Network/SimGsm/SimGsmModem.h"
#include "Core/Assert.h"
#include <string.h>

static uint8_t ib[2048], ob[1024];
static SimGsmModem* gsm;

int simGsmTcpEchoTest()
{
	Usart* usart = Usart::instance( &SD6 );
	usart->setOutputBuffer( ob, 1024 );
	usart->setInputBuffer( ib, 2048 );
	usart->open();
	palSetPadMode( GPIOC, 6, PAL_MODE_ALTERNATE( GPIO_AF_USART6 ) );
	palSetPadMode( GPIOC, 7, PAL_MODE_ALTERNATE( GPIO_AF_USART6 ) );

	gsm = new SimGsmModem( IOPA8 );
	gsm->setUsart( usart );
	gsm->setApn( "m2m30.velcom.by" );
	gsm->startModem( NORMALPRIO );
	gsm->waitForStateChange();

	AbstractTcpServer* server = gsm->tcpServer( 0 );
	server->setNewConnectionsBufferSize( 128, 128 );

	struct Desc
	{
		EvtListener listener;
		AbstractTcpSocket* socket;
	} static socketDesc[SIMGSM_SOCKET_LIMIT];
	for( int i = 0; i < SIMGSM_SOCKET_LIMIT; ++i )
		socketDesc[i].socket = nullptr;
	uint32_t numActiveClients = 0;

	eventflags_t flags;
	EvtListener serverListener;
	server->eventSource()->registerMask( &serverListener, EVENT_MASK( 31 ) );
	server->listen( 502 );
	while( true )
	{
		eventmask_t em = chEvtWaitAny( ALL_EVENTS );

		// socket events
		for( int i = 0; i < SIMGSM_SOCKET_LIMIT; ++i )
		{
			if( em & EVENT_MASK( i ) )
			{
				flags = socketDesc[i].listener.getAndClearFlags();
				AbstractSocket* socket = socketDesc[i].socket;

				if( flags & ( eventflags_t )SocketEventFlag::Error )
				{
					socket->eventSource()->unregister( &socketDesc[i].listener );
					delete socketDesc[i].socket;
					socketDesc[i].socket = nullptr;
					--numActiveClients;
					continue;
				}
				if( flags & ( eventflags_t )SocketEventFlag::InputAvailable )
				{
					uint8_t* data[2];
					uint32_t dataSize[2];
					ringToLinearArrays( socket->inputBuffer()->begin(), socket->readAvailable(), data, dataSize, data + 1, dataSize + 1 );
					socket->write( data[0], dataSize[0], TIME_INFINITE );
					socket->write( data[1], dataSize[1], TIME_INFINITE );
					socket->read( nullptr, dataSize[0] + dataSize[1], TIME_INFINITE );
				}
			}
		}

		// server events
		if( em & EVENT_MASK( 31 ) )
		{
			flags = serverListener.getAndClearFlags();
			if( flags & ( eventflags_t )TcpServerEventFlag::Error )
			{
				AbstractTcpSocket* socket;
				while( ( socket = server->nextPendingConnection() ) )
					delete socket;

				gsm->waitForStateChange();
				if( gsm->state() == ModemState::Stopped )
					assert( false ); //chSysHalt( "GG" );
				server->listen( 502 );
				continue;
			}
			if( flags & ( eventflags_t )TcpServerEventFlag::NewConnection )
			{
				AbstractTcpSocket* newSocket;
				while( ( newSocket = server->nextPendingConnection() ) )
				{
					if( numActiveClients == 3 )
					{
						newSocket->close();
						delete newSocket;
						continue;
					}
					for( int i = 0; i < SIMGSM_SOCKET_LIMIT; ++i )
					{
						if( !socketDesc[i].socket )
						{
							socketDesc[i].socket = newSocket;
							++numActiveClients;
							newSocket->eventSource()->registerMask( &socketDesc[i].listener, EVENT_MASK( i ) );
							if( newSocket->readAvailable() )
								newSocket->eventSource()->broadcastFlags( ( eventflags_t )SocketEventFlag::InputAvailable );
							break;
						}
					}
				}
			}
		}
	}

	return 0;
}