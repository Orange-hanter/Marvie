#pragma once

#include "AbstractUdpSocket.h"
#include "LwipSocketPrivate.h"

class UdpSocket : public AbstractUdpSocket, private LwipSocketPrivate
{
public:
	UdpSocket();
	~UdpSocket();

	bool waitDatagram( sysinterval_t timeout ) override;
	bool hasPendingDatagrams() const override;
	uint32_t pendingDatagramSize() const override;
	uint32_t readDatagram( uint8_t* data, uint32_t maxSize, IpAddress* address = nullptr, uint16_t* port = nullptr ) override;

	uint32_t writeDatagram( const uint8_t* data, uint32_t size, IpAddress address, uint16_t port ) override;
	uint32_t writeDatagram( const uint8_t* data, uint32_t size ) override;

	bool bind( uint16_t port ) override;
	bool bind( IpAddress address, uint16_t port ) override;
	bool connect( const char* hostName, uint16_t port ) override;
	bool connect( IpAddress address, uint16_t port ) override;
	void disconnect() override;

	SocketState state() override;
	SocketError socketError() const override;

	bool open() override;
	bool isOpen() const override;
	void reset() override;
	void close() override;

	uint32_t write( const uint8_t* data, uint32_t size, sysinterval_t timeout = TIME_INFINITE ) override;
	uint32_t read( uint8_t* data, uint32_t size, sysinterval_t timeout = TIME_INFINITE ) override;
	uint32_t peek( uint32_t pos, uint8_t* data, uint32_t size ) override; // only for the first datagram
	uint32_t readAvailable() const override;
	bool waitForBytesWritten( sysinterval_t timeout ) override;
	bool waitForReadAvailable( uint32_t size, sysinterval_t timeout ) override;

	AbstractReadable* inputBuffer() override;
	AbstractWritable* outputBuffer() override;

	EvtSource* eventSource() override;
};