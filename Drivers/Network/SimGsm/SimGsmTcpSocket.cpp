#include "SimGsmTcpSocket.h"
#include "Core/Assert.h"

SimGsmTcpSocket::SimGsmTcpSocket( SimGsm* simGsm, uint32_t inputBufferSize, uint32_t outputBufferSize ) : SimGsmSocketBase( simGsm, inputBufferSize, outputBufferSize )
{

}

SimGsmTcpSocket::~SimGsmTcpSocket()
{
	close();
}

bool SimGsmTcpSocket::open()
{
	return false;
}

bool SimGsmTcpSocket::isOpen() const
{
	return linkId != -1;
}

void SimGsmTcpSocket::reset()
{
	assert( false );
}

void SimGsmTcpSocket::close()
{
	gsm->closeSocket( this );
}

uint32_t SimGsmTcpSocket::write( const uint8_t* data, uint32_t size, sysinterval_t timeout )
{
	assert( timeout == TIME_INFINITE );

	return gsm->sendSocketData( this, data, size );
}

uint32_t SimGsmTcpSocket::read( uint8_t* data, uint32_t size, sysinterval_t timeout )
{
	return inBuffer.read( data, size, timeout );
}

uint32_t SimGsmTcpSocket::readAvailable() const
{
	return inBuffer.readAvailable();
}

bool SimGsmTcpSocket::waitForReadAvailable( uint32_t size, sysinterval_t timeout )
{
	return inBuffer.waitForReadAvailable( size, timeout );
}

AbstractReadable* SimGsmTcpSocket::inputBuffer()
{
	return &inBuffer;
}

AbstractWritable* SimGsmTcpSocket::outputBuffer()
{
	assert( false );
	return nullptr;
}

EvtSource* SimGsmTcpSocket::eventSource()
{
	return &eSource;
}

bool SimGsmTcpSocket::bind( uint16_t port )
{
	assert( false );
	return false;
}

bool SimGsmTcpSocket::connect( const char* hostName, uint16_t port )
{
	assert( false );
	return false;
}

bool SimGsmTcpSocket::connect( IpAddress address, uint16_t port )
{
	if( sState != SocketState::Unconnected )
		return false;
	resetHelp();
	return gsm->openTcpSocket( this, address, port );
}

void SimGsmTcpSocket::disconnect()
{
	gsm->closeSocket( this );
}

SocketState SimGsmTcpSocket::state()
{
	return sState;
}

SocketError SimGsmTcpSocket::socketError() const
{
	return sError;
}
