#pragma once

#include "SimGsmSocketBase.h"

class SimGsmUdpSocket : protected SimGsmSocketBase, public AbstractUdpSocket
{
	friend class SimGsm;
	SimGsmUdpSocket( SimGsm* simGsm, uint32_t inputBufferSize, uint32_t outputBufferSize );

public:
	~SimGsmUdpSocket();

	bool open() final override;
	bool isOpen() const final override;
	void reset() final override; // not supported
	void close() final override;

	uint32_t write( const uint8_t* data, uint32_t size, sysinterval_t timeout ) override;
	uint32_t read( uint8_t* data, uint32_t size, sysinterval_t timeout ) override;
	uint32_t readAvailable() const override;
	bool waitForBytesWritten( sysinterval_t timeout ) final override;
	bool waitForReadAvailable( uint32_t size, sysinterval_t timeout ) final override; // not supported, use waitDatagram() instead

	AbstractReadable* inputBuffer() final override;
	AbstractWritable* outputBuffer() final override;

	EvtSource* eventSource() final override;

	bool bind( uint16_t port ) final override;
	bool connect( const char* hostName, uint16_t port ) final override;
	bool connect( IpAddress address, uint16_t port ) final override;
	void disconnect() final override;

	SocketState state() final override;
	SocketError socketError() const final override;

	bool waitDatagram( sysinterval_t timeout ) final override;
	bool hasPendingDatagrams() const final override;
	uint32_t pendingDatagramSize() const final override;
	uint32_t readDatagram( uint8_t* data, uint32_t maxSize, IpAddress* address = nullptr, uint16_t* port = nullptr ) final override;

	uint32_t writeDatagram( const uint8_t* data, uint32_t size, IpAddress address, uint16_t port ) final override;
	uint32_t writeDatagram( const uint8_t* data, uint32_t size ) final override;

private:	
	IpAddress rAddr, cAddr;
	uint16_t rPort, cPort;
	typedef UdpPacketHeader Header;
};