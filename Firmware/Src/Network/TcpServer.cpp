#include "TcpServer.h"
#include "TcpSocket.h"
#include "LwipSocketPrivate.h"
#include "lwip/tcpip.h"
#include "lwip/tcp.h"

#if defined( LWIP_TCPIP_CORE_LOCKING ) && !defined( CH_CFG_USE_MUTEXES_RECURSIVE )
#error "Need recursive mutexes"
#endif

BinarySemaphore TcpServer::sem( false );
netconn* TcpServer::blockCon = nullptr;

TcpServer::TcpServer()
{
	con = netconn_new_with_callback( NETCONN_TCP, netconnCallback );
	con->socket = ( int )this;
	newcon = nullptr;
	recvCounter = 0;
	prevState = false;
}

TcpServer::~TcpServer()
{
	deleteAll();
}

bool TcpServer::isListening() const
{
	return con->pcb.tcp && ( con->pcb.tcp->state == tcp_state::LISTEN ||
							 con->pcb.tcp->state == tcp_state::SYN_RCVD ||
							 con->pcb.tcp->state == tcp_state::ESTABLISHED );
}

bool TcpServer::listen( uint16_t port, IpAddress address )
{
	if( !con->pcb.tcp || con->pcb.tcp->state != tcp_state::CLOSED )
		recreate();
	ip_addr_t addr = IPADDR4_INIT_BYTES( address.addr[3], address.addr[2], address.addr[1], address.addr[0] );
	volatile bool res = netconn_bind( con, &addr, port ) == ERR_OK && netconn_listen( con ) == ERR_OK;
	updateState();

	return res;
}

SocketError TcpServer::serverError() const
{
	return LwipSocketPrivate::socketError( con );
}

void TcpServer::close()
{
	deleteAll();
}

TcpSocket* TcpServer::nextPendingConnection()
{
	if( !newcon )
	{
		if( recvCounter <= 0 )
			return nullptr;
		takeNewCon();
		if( !newcon )
			return nullptr;
	}

#ifdef LWIP_TCPIP_CORE_LOCKING
	LOCK_TCPIP_CORE();
#else
	tprio_t prio = chThdSetPriority( TCPIP_THREAD_PRIO );
	chThdYield();
#endif
	TcpSocket* tcpSocket;
	if( newcon->socket == -1 )
	{
		tcpSocket = new TcpSocket( 0 );
		if( newcon->socket != -1 )
		{
			delete tcpSocket;
			tcpSocket = ( TcpSocket* )newcon->socket;
		}
		else
			tcpSocket->setNetconn( newcon );
	}
	else
		tcpSocket = static_cast< TcpSocket* >( ( LwipSocketPrivate* )newcon->socket );
#ifdef LWIP_TCPIP_CORE_LOCKING
	UNLOCK_TCPIP_CORE();
#else
	chThdSetPriority( prio );
#endif
	newcon = nullptr;

	return tcpSocket;
}

bool TcpServer::waitForNewConnection( sysinterval_t timeout /*= TIME_INFINITE */ )
{
	if( newcon )
		return true;
	if( recvCounter > 0 )
		goto Recv;
	if( timeout == TIME_IMMEDIATE )
		return false;

	if( timeout == TIME_INFINITE )
		con->recv_timeout = 0;
	else
	{
		int t = ( int )TIME_I2MS( timeout );
		if( t == 0 )
			con->recv_timeout = 1;
		else
			con->recv_timeout = t;
	}

Recv:
	takeNewCon();
	return newcon != nullptr;
}

void TcpServer::setNewConnectionsBufferSize( uint32_t inputBufferSize, uint32_t outputBufferSize )
{

}

EventSourceRef TcpServer::eventSource()
{
	return &evtSource;
}

void TcpServer::recreate()
{
	deleteAll();
	con = netconn_new_with_callback( NETCONN_TCP, netconnCallback );
	con->socket = ( int )this;
	newcon = nullptr;
	recvCounter = 0;
}

void TcpServer::deleteAll()
{
	if( !con )
		return;

#ifdef LWIP_TCPIP_CORE_LOCKING
	LOCK_TCPIP_CORE();
#else
	tprio_t prio = chThdSetPriority( TCPIP_THREAD_PRIO );
	chThdYield();
#endif

	con->callback = nullptr;
	if( newcon )
		deleteNewConP();
	while( recvCounter )
	{
		takeNewCon();
		if( newcon == nullptr )
			break;
		deleteNewConP();
	}
#ifdef LWIP_TCPIP_CORE_LOCKING
	UNLOCK_TCPIP_CORE();
#else
	chThdSetPriority( prio );
#endif

	netconn_delete( con );
	con = nullptr;
	newcon = nullptr;
}

void TcpServer::takeNewCon()
{
	if( netconn_accept( con, &newcon ) == ERR_OK )
	{
		chSysLock();
		--recvCounter;
		chSysUnlock();
	}
}

void TcpServer::deleteNewConP()
{
	newcon->callback = nullptr;
	if( newcon->socket == -1 )
		netconn_delete( newcon );
	else
		delete ( TcpSocket* )newcon->socket;
}

void TcpServer::updateState()
{
	chSysLock();
	updateStateS();
	chSchRescheduleS();
	chSysUnlock();
}

void TcpServer::updateStateS()
{
	auto current = isListening();
	if( prevState != current )
	{
		evtSource.broadcastFlags( ( eventflags_t )SocketEventFlag::StateChanged );
		prevState = current;
	}
}

void TcpServer::netconnCallback( netconn* con, netconn_evt evt, u16_t len )
{
	if( con->socket == -1 )
	{
		TcpSocket* tcpSocket = new TcpSocket( 0 );
		if( con->socket != -1 )
			delete tcpSocket;
		else
			tcpSocket->setNetconn( con );
		LwipSocketPrivate::netconnCallback( con, evt, len );

		return;
	}
	TcpServer* server = ( ( TcpServer* )con->socket );
	if( evt == NETCONN_EVT_RCVPLUS )
	{
		chSysLock();
		++server->recvCounter;
		server->evtSource.broadcastFlags( ( eventflags_t )TcpServerEventFlag::NewConnection );
		chSchRescheduleS();
		chSysUnlock();
	}
}