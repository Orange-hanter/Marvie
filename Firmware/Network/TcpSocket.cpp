#include "TcpSocket.h"
#include "Core/Assert.h"
#include "Lib/lwip/src/include/lwip/tcp.h"
#include <string.h>

TcpSocket::TcpSocket() : LwipSocketPrivate( NETCONN_TCP )
{

}

TcpSocket::TcpSocket( uint32_t ) : LwipSocketPrivate( 0 )
{

}

TcpSocket::~TcpSocket()
{

}

bool TcpSocket::bind( uint16_t port )
{
	return LwipSocketPrivate::bind( port );
}

bool TcpSocket::bind( IpAddress address, uint16_t port )
{
	return LwipSocketPrivate::bind( address, port );
}

bool TcpSocket::connect( const char* hostName, uint16_t port )
{
	return LwipSocketPrivate::connect( hostName, port );
}

bool TcpSocket::connect( IpAddress address, uint16_t port )
{
	return LwipSocketPrivate::connect( address, port );
}

void TcpSocket::disconnect()
{
	netconn_close( con );
}

SocketState TcpSocket::state()
{
	return LwipSocketPrivate::state();
}

SocketError TcpSocket::socketError() const
{
	return LwipSocketPrivate::socketError();
}

bool TcpSocket::open()
{
	return false;
}

bool TcpSocket::isOpen() const
{
	return LwipSocketPrivate::isOpen();
}

void TcpSocket::reset()
{
	assert( false );
}

void TcpSocket::close()
{
	netconn_close( con );
}

uint32_t TcpSocket::write( const uint8_t* data, uint32_t size, sysinterval_t timeout /*= TIME_INFINITE */ )
{
	assert( timeout == TIME_INFINITE );
	if( size == 0 )
		return 0;
	if( netconn_write( con, data, size, NETCONN_COPY ) == ERR_OK )
		return size;
	return 0;
}

uint32_t TcpSocket::read( uint8_t* data, uint32_t size, sysinterval_t timeout /*= TIME_INFINITE */ )
{
	return LwipSocketPrivate::read( data, size, timeout );
}

uint32_t TcpSocket::peek( uint32_t pos, uint8_t* data, uint32_t size )
{
	uint32_t n = readAvailable();
	if( n <= pos )
		return 0;
	if( !lastNetbuf )
		takeNetbuf();
	uint32_t m = n + offset;
	while( lastNetbuf->p->tot_len < m )
		catNetbuf();

	return LwipSocketPrivate::netbufPeek( pos, data, size );
}

uint32_t TcpSocket::readAvailable() const
{
	return LwipSocketPrivate::readAvailable();
}

bool TcpSocket::waitForBytesWritten( sysinterval_t timeout )
{
	return true;
}

bool TcpSocket::waitForReadAvailable( uint32_t size, sysinterval_t timeout )
{
	return LwipSocketPrivate::waitForReadAvailable( size, timeout );
}

AbstractReadable* TcpSocket::inputBuffer()
{
	return nullptr;
}

AbstractWritable* TcpSocket::outputBuffer()
{
	return nullptr;
}

EvtSource* TcpSocket::eventSource()
{
	return &evtSource;
}