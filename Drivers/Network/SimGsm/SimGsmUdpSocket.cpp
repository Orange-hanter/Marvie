#include "SimGsmUdpSocket.h"
#include "Support/Utility.h"
#include "Core/Assert.h"

#if CH_CFG_INTERVALS_SIZE != CH_CFG_TIME_TYPES_SIZE
#error "CH_CFG_INTERVALS_SIZE != CH_CFG_TIME_TYPES_SIZE"
#endif

SimGsmUdpSocket::SimGsmUdpSocket( SimGsm* simGsm, uint32_t inputBufferSize, uint32_t outputBufferSize ) : SimGsmSocketBase( simGsm, inputBufferSize, outputBufferSize )
{
	cAddr = rAddr = IpAddress( 0, 0, 0, 0 );
	cPort = rPort = 0;
}

SimGsmUdpSocket::~SimGsmUdpSocket()
{
	close();
}

bool SimGsmUdpSocket::open()
{
	return false;
}

bool SimGsmUdpSocket::isOpen() const
{
	return linkId != -1;
}

void SimGsmUdpSocket::reset()
{
	assert( false );
}

void SimGsmUdpSocket::close()
{
	gsm->closeSocket( this );
}

uint32_t SimGsmUdpSocket::write( const uint8_t* data, uint32_t size, sysinterval_t timeout )
{
	assert( timeout == TIME_INFINITE );

	return writeDatagram( data, size );
}

uint32_t SimGsmUdpSocket::read( uint8_t* data, uint32_t size, sysinterval_t timeout )
{
	assert( timeout == TIME_INFINITE );

	return readDatagram( data, size, nullptr, nullptr );
}

uint32_t SimGsmUdpSocket::readAvailable() const
{
	if( hasPendingDatagrams() )
		return pendingDatagramSize();
	return 0;
}

bool SimGsmUdpSocket::waitForReadAvailable( uint32_t size, sysinterval_t timeout )
{
	assert( false );
	return false;
}

AbstractReadable* SimGsmUdpSocket::inputBuffer()
{
	return &inBuffer;
}

AbstractWritable* SimGsmUdpSocket::outputBuffer()
{
	assert( false );
	return nullptr;
}

EvtSource* SimGsmUdpSocket::eventSource()
{
	return &eSource;
}

bool SimGsmUdpSocket::bind( uint16_t port )
{
	if( sState != SocketState::Unconnected )
		return false;
	resetHelp();
	cAddr = IpAddress( 8, 8, 8, 8 );
	cPort = 8888;
	return gsm->openUdpSocket( this, port );
}

bool SimGsmUdpSocket::connect( const char* hostName, uint16_t port )
{
	assert( false );
	return false;
}

bool SimGsmUdpSocket::connect( IpAddress address, uint16_t port )
{
	rAddr = address;
	rPort = port;
	return true;
}

void SimGsmUdpSocket::disconnect()
{
	rAddr = IpAddress( 0, 0, 0, 0 );
	rPort = 0;
}

SocketState SimGsmUdpSocket::state()
{
	return sState;
}

SocketError SimGsmUdpSocket::socketError() const
{
	return sError;
}

bool SimGsmUdpSocket::waitDatagram( sysinterval_t timeout )
{
	if( timeout == TIME_INFINITE || timeout == TIME_IMMEDIATE )
	{
		if( inBuffer.waitForReadAvailable( sizeof( Header ), timeout ) )
		{
			Header header;
			Utility::copy( reinterpret_cast< uint8_t* >( &header ), inBuffer.begin(), sizeof( header ) );
			return inBuffer.waitForReadAvailable( sizeof( Header ) + header.datagramSize, timeout );
		}
		return false;
	}
	else
	{
		systime_t deadline = chVTGetSystemTimeX() + ( systime_t )timeout;
		if( !inBuffer.waitForReadAvailable( sizeof( Header ), timeout ) )
			return false;

		sysinterval_t nextTimeout = ( sysinterval_t )( deadline - chVTGetSystemTimeX() );
		if( nextTimeout > timeout ) // timeout expired
			return false;

		Header header;
		Utility::copy( reinterpret_cast< uint8_t* >( &header ), inBuffer.begin(), sizeof( header ) );
		return inBuffer.waitForReadAvailable( sizeof( Header ) + header.datagramSize, nextTimeout );
	}
}

bool SimGsmUdpSocket::hasPendingDatagrams() const
{
	if( inBuffer.readAvailable() <= sizeof( Header ) )
		return false;
	Header header;
	Utility::copy( reinterpret_cast< uint8_t* >( &header ), const_cast< DynamicByteRingBuffer* >( &inBuffer )->begin(), sizeof( header ) );

	return inBuffer.readAvailable() >= sizeof( header ) + header.datagramSize;
}

uint32_t SimGsmUdpSocket::pendingDatagramSize() const
{
	Header header;
	Utility::copy( reinterpret_cast< uint8_t* >( &header ), const_cast< DynamicByteRingBuffer* >( &inBuffer )->begin(), sizeof( header ) );

	return header.datagramSize;
}

uint32_t SimGsmUdpSocket::readDatagram( uint8_t* data, uint32_t maxSize, IpAddress* address /*= nullptr*/, uint16_t* port /*= nullptr */ )
{
	assert( hasPendingDatagrams() == true );

	Header header;
	inBuffer.read( reinterpret_cast< uint8_t* >( &header ), sizeof( header ) );
	if( address )
		*address = header.remoteAddress;
	if( port )
		*port = header.remotePort;
	if( header.datagramSize > maxSize )
	{
		inBuffer.read( data, maxSize );
		inBuffer.read( nullptr, header.datagramSize - maxSize );
		return maxSize;
	}

	inBuffer.read( data, header.datagramSize );
	return header.datagramSize;
}

uint32_t SimGsmUdpSocket::writeDatagram( const uint8_t* data, uint32_t size, IpAddress address, uint16_t port )
{
	return gsm->sendSocketData( this, data, size, address, port );
}

uint32_t SimGsmUdpSocket::writeDatagram( const uint8_t* data, uint32_t size )
{
	if( rPort )
		return gsm->sendSocketData( this, data, size, rAddr, rPort );
	return 0;
}