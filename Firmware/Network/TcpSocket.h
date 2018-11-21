#pragma once

#include "AbstractTcpSocket.h"
#include "LwipSocketPrivate.h"

class TcpSocket : public AbstractTcpSocket, private LwipSocketPrivate
{
	friend class TcpServer;
	TcpSocket( uint32_t );

public:
	TcpSocket();
	~TcpSocket();

	bool bind( uint16_t port ) override;
	bool bind( IpAddress address, uint16_t port ) override;
	bool connect( const char* hostName, uint16_t port ) override;
	bool connect( IpAddress address, uint16_t port ) override;
	void disconnect() override;

	SocketState state() override;
	SocketError socketError() const override;

	void setSocketOption( SocketOption option, int value ) override;
	int socketOption( SocketOption option ) override;

	bool open() override;
	bool isOpen() const override;
	void reset() override;
	void close() override;

	uint32_t write( const uint8_t* data, uint32_t size, sysinterval_t timeout = TIME_INFINITE ) override;
	uint32_t read( uint8_t* data, uint32_t size, sysinterval_t timeout = TIME_INFINITE ) override;
	uint32_t peek( uint32_t pos, uint8_t* data, uint32_t size ) override;
	uint32_t readAvailable() const override;
	bool waitForBytesWritten( sysinterval_t timeout ) override;
	bool waitForReadAvailable( uint32_t size, sysinterval_t timeout ) override;

	AbstractReadable* inputBuffer() override;
	AbstractWritable* outputBuffer() override;

	EvtSource* eventSource() override;
};