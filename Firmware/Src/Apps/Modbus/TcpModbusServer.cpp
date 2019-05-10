#include "TcpModbusServer.h"

TcpModbusServer::TcpModbusServer( uint32_t stackSize /*= TCP_MODBUS_SERVER_STACK_SIZE */ ) : AbstractModbusNetworkServer( stackSize )
{
	clients = nullptr;
	inactivityTimeout = TIME_S2I( 60 * 10 );
}

TcpModbusServer::~TcpModbusServer()
{
	stopServer();
	waitForStateChange();
}

void TcpModbusServer::setClientInactivityTimeout( sysinterval_t timeout )
{
	inactivityTimeout = timeout;
}

void TcpModbusServer::main()
{
	clients = new Client*[maxClientsCount];
	if( !clients )
		goto End;
	for( int i = 0; i < maxClientsCount; ++i )
		clients[i] = nullptr;
	tcpServer.eventSource().registerMask( &listener, InnerEventFlag::ServerSocketFlag );
	chEvtAddEvents( InnerEventFlag::ServerSocketFlag );

	while( true )
	{
		eventmask_t em = chEvtWaitAny( ALL_EVENTS );
		if( em & InnerEventFlag::StopRequestFlag )
			break;
		if( em & InnerEventFlag::ServerSocketFlag )
		{
			while( TcpSocket* newConnection = tcpServer.nextPendingConnection() )
				addNewClient( newConnection );
			if( !tcpServer.isListening() )
			{
				if( !tcpServer.listen( port ) )
				{
					addEvents( InnerEventFlag::ServerSocketFlag );
					chThdSleepMilliseconds( 20 );
				}
			}
		}
		em &= ~( InnerEventFlag::ServerSocketFlag | InnerEventFlag::StopRequestFlag );
		for( uint32_t i = 0; em; ++i, em >>= 1 )
		{
			if( em & 1 && clients[i] )
			{
				if( !clients[i]->stream->socket->isOpen() || clients[i]->needClose )
				{
					delete clients[i];
					clients[i] = nullptr;
					--clientsCount;
					eSource.broadcastFlags( EventFlag::ClientsCountChanged );
					continue;
				}
				unsigned int nextInterval = clients[i]->framer->poll();
				if( nextInterval )
				{
					clients[i]->requestWaiting = false;
					clients[i]->timer.start( ( sysinterval_t )nextInterval );
				}
				else
				{
					clients[i]->requestWaiting = true;
					clients[i]->timer.start( inactivityTimeout );
				}
			}
		}
	}

End:
	tcpServer.close();
	if( clientsCount )
	{
		for( int i = 0; i < maxClientsCount; ++i )
		{
			if( clients[i] )
				delete clients[i];
		}
	}
	delete clients;
	clientsCount = 0;
	clients = nullptr;

	chSysLock();
	sState = State::Stopped;
	eSource.broadcastFlags( ( eventflags_t )EventFlag::StateChanged );
	waitingQueue.dequeueNext( MSG_OK );
}

void TcpModbusServer::addNewClient( TcpSocket* socket )
{
	if( clientsCount >= maxClientsCount )
	{
		delete socket;
		return;
	}
	for( int i = 0; i < maxClientsCount; ++i )
	{
		if( clients[i] )
			continue;
		clients[i] = new Client;
		if( !clients[i] )
		{
			delete socket;
			return;
		}
		clients[i]->stream = new SocketStream( socket );
		switch( frameType )
		{
		case FrameType::Rtu:
			clients[i]->buffer = new uint8_t[MODBUS_RTU_DATA_BUFFER_SIZE];
			clients[i]->framer = new ModbusPotato::CModbusRTU( clients[i]->stream, &timerProvider, clients[i]->buffer, MODBUS_RTU_DATA_BUFFER_SIZE );
			static_cast< ModbusPotato::CModbusRTU* >( clients[i]->framer )->setup( 115200 ); // dummy value
			break;
		case FrameType::Ascii:
			clients[i]->buffer = new uint8_t[MODBUS_ASCII_DATA_BUFFER_SIZE];
			clients[i]->framer = new ModbusPotato::CModbusASCII( clients[i]->stream, &timerProvider, clients[i]->buffer, MODBUS_ASCII_DATA_BUFFER_SIZE );
			break;
		case FrameType::Ip:
			clients[i]->buffer = new uint8_t[MODBUS_IP_DATA_BUFFER_SIZE];
			clients[i]->framer = new ModbusPotato::CModbusIP( clients[i]->stream, &timerProvider, clients[i]->buffer, MODBUS_IP_DATA_BUFFER_SIZE );
			break;
		default:
			break;
		}
		if( clients[i]->stream == nullptr || clients[i]->buffer == nullptr || clients[i]->framer == nullptr )
		{
			if( clients[i]->stream == nullptr )
				delete socket;
			delete clients[i];
			clients[i] = nullptr;
			return;
		}
		clients[i]->framer->set_handler( &slave );
		socket->eventSource().registerMask( &clients[i]->listener, EVENT_MASK( i ) );
		chEvtAddEvents( EVENT_MASK( i ) );
		++clientsCount;
		eSource.broadcastFlags( EventFlag::ClientsCountChanged );
		break;
	}
}

void TcpModbusServer::timerCallback( Client* client )
{
	chSysLockFromISR();
	if( client->requestWaiting )
		client->needClose = true;
	client->listener.thread().signalEventsI( client->listener.eventMask() );
	chSysUnlockFromISR();
}

TcpModbusServer::SocketStream::SocketStream( TcpSocket* socket ) : socket( socket )
{

}

TcpModbusServer::SocketStream::~SocketStream()
{
	if( socket->isOpen() )
		socket->disconnect();
	delete socket;
}

int TcpModbusServer::SocketStream::read( uint8_t* buffer, size_t buffer_size )
{
	uint32_t a = socket->readAvailable();
	if( a == 0 )
		return 0;

	if( buffer_size != ( size_t )-1 && a > buffer_size )
		a = buffer_size;

	if( !buffer )
	{
		socket->read( nullptr, a, TIME_IMMEDIATE );
		return a;
	}

	return socket->read( buffer, a, TIME_IMMEDIATE );
}

int TcpModbusServer::SocketStream::write( uint8_t* buffer, size_t len )
{
	return socket->write( buffer, len, TIME_INFINITE );
}

void TcpModbusServer::SocketStream::txEnable( bool state )
{

}

bool TcpModbusServer::SocketStream::writeComplete()
{
	return true;
}

void TcpModbusServer::SocketStream::communicationStatus( bool rx, bool tx )
{

}

TcpModbusServer::Client::Client()
{
	stream = nullptr;
	buffer = nullptr;
	framer = nullptr;
	timer.setParameter( this );
	requestWaiting = false;
	needClose = false;
}

TcpModbusServer::Client::~Client()
{
	delete stream;
	delete buffer;
	delete framer;
}