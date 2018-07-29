#include "UdpSocket.h"
#include "Core/Assert.h"
#include "Lib/lwip/src/include/lwip/udp.h"
#include <string.h>

UdpSocket::UdpSocket() : LwipSocketPrivate( NETCONN_UDP )
{

}

UdpSocket::~UdpSocket()
{

}

bool UdpSocket::waitDatagram( sysinterval_t timeout )
{
	if( lastNetbuf )
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
	return takeNetbuf() > 0;
}

bool UdpSocket::hasPendingDatagrams() const
{
	return lastNetbuf || recvCounter > 0;
}

uint32_t UdpSocket::pendingDatagramSize() const
{
	if( lastNetbuf )
		return lastNetbuf->p->tot_len;

	if( recvCounter > 0 )
		return const_cast< UdpSocket* >( this )->takeNetbuf();

	return 0;
}

uint32_t UdpSocket::readDatagram( uint8_t* data, uint32_t maxSize, IpAddress* address /*= nullptr*/, uint16_t* port /*= nullptr */ )
{
	uint8_t* p = data;
	if( lastNetbuf == nullptr )
	{
		if( recvCounter <= 0 )
			return 0;
		if( takeNetbuf() > 0 )
			goto Read;
	}
	else
	{
Read:
		if( maxSize > lastNetbuf->p->tot_len )
			maxSize = lastNetbuf->p->tot_len;
		lastNetbuf->ptr = lastNetbuf->p;
		while( maxSize )
		{
			uint8_t* tmp;
			u16_t len;
			netbuf_data( lastNetbuf, ( void** )&tmp, &len );
			if( len > maxSize )
				len = maxSize;
			memcpy( p, tmp, len );
			p += len;
			maxSize -= len;
			netbuf_next( lastNetbuf );
		}
		if( address )
			*address = IpAddress( lastNetbuf->addr.addr, lastNetbuf->addr.addr >> 8,
								  lastNetbuf->addr.addr >> 16, lastNetbuf->addr.addr >> 24 );
		if( port )
			*port = lastNetbuf->port;
		netbuf_delete( lastNetbuf );
		lastNetbuf = nullptr;
		offset = 0;

		return p - data;
	}

	return 0;
}

uint32_t UdpSocket::writeDatagram( const uint8_t* data, uint32_t size, IpAddress address, uint16_t port )
{
	if( size == 0 )
		return 0;

	netbuf* buf = netbuf_new();
	if( buf == nullptr )
		return 0;
	ip_addr_t addr = IPADDR4_INIT_BYTES( address.addr[3], address.addr[2], address.addr[1], address.addr[0] );
	bool noErr = netbuf_ref( buf, data, ( u16_t )size ) == ERR_OK && netconn_sendto( con, buf, &addr, port ) == ERR_OK;
	netbuf_delete( buf );

	return noErr ? ( uint16_t )size : 0;
}

uint32_t UdpSocket::writeDatagram( const uint8_t* data, uint32_t size )
{
	if( size == 0 )
		return 0;

	netbuf* buf = netbuf_new();
	if( buf == nullptr )
		return 0;
	bool noErr = netbuf_ref( buf, data, ( u16_t )size ) == ERR_OK && netconn_send( con, buf ) == ERR_OK;
	netbuf_delete( buf );

	return noErr ? ( uint16_t )size : 0;
}

bool UdpSocket::bind( uint16_t port )
{
	return LwipSocketPrivate::bind( port );
}

bool UdpSocket::bind( IpAddress address, uint16_t port )
{
	return LwipSocketPrivate::bind( address, port );
}

bool UdpSocket::connect( const char* hostName, uint16_t port )
{
	return LwipSocketPrivate::connect( hostName, port );
}

bool UdpSocket::connect( IpAddress address, uint16_t port )
{
	return LwipSocketPrivate::connect( address, port );
}

void UdpSocket::disconnect()
{
	netconn_disconnect( con );
}

SocketState UdpSocket::state()
{
	return LwipSocketPrivate::state();
}

SocketError UdpSocket::socketError() const
{
	return LwipSocketPrivate::socketError();
}

bool UdpSocket::open()
{
	return false;
}

bool UdpSocket::isOpen() const
{
	return LwipSocketPrivate::isOpen();
}

void UdpSocket::reset()
{
	assert( false );
}

void UdpSocket::close()
{
	netconn_disconnect( con );
}

uint32_t UdpSocket::write( const uint8_t* data, uint32_t size, sysinterval_t timeout /*= TIME_INFINITE */ )
{
	assert( timeout == TIME_INFINITE );
	return writeDatagram( data, size );
}

uint32_t UdpSocket::read( uint8_t* data, uint32_t size, sysinterval_t timeout /*= TIME_INFINITE */ )
{
	return LwipSocketPrivate::read( data, size, timeout );
}

uint32_t UdpSocket::peek( uint32_t pos, uint8_t* data, uint32_t size )
{
	if( pendingDatagramSize() == 0 )
		return 0;

	return LwipSocketPrivate::netbufPeek( pos, data, size );
}

uint32_t UdpSocket::readAvailable() const
{
	return LwipSocketPrivate::readAvailable();
}

bool UdpSocket::waitForBytesWritten( sysinterval_t timeout )
{
	return true;
}

bool UdpSocket::waitForReadAvailable( uint32_t size, sysinterval_t timeout )
{
	return LwipSocketPrivate::waitForReadAvailable( size, timeout );
}

AbstractReadable* UdpSocket::inputBuffer()
{
	return nullptr;
}

AbstractWritable* UdpSocket::outputBuffer()
{
	return nullptr;
}

EvtSource* UdpSocket::eventSource()
{
	return &evtSource;
}