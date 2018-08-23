#pragma once

#include "SimGsmSocketBase.h"

class SimGsmTcpSocket : protected SimGsmSocketBase, public AbstractTcpSocket
{
	friend class SimGsmModem;
	SimGsmTcpSocket( SimGsmModem* simGsm, uint32_t inputBufferSize, uint32_t outputBufferSize );

public:
	~SimGsmTcpSocket();

	bool open() final override;
	bool isOpen() const final override;
	void reset() final override;
	void close();

	uint32_t write( const uint8_t* data, uint32_t size, sysinterval_t timeout ) override;
	uint32_t read( uint8_t* data, uint32_t size, sysinterval_t timeout ) override;
	uint32_t peek( uint32_t pos, uint8_t* data, uint32_t size ) override;
	uint32_t readAvailable() const override;
	bool waitForBytesWritten( sysinterval_t timeout ) final override;
	bool waitForReadAvailable( uint32_t size, sysinterval_t timeout ) final override;

	AbstractReadable* inputBuffer() final override;
	AbstractWritable* outputBuffer() final override;

	EvtSource* eventSource() final override;

	bool bind( uint16_t port ) final override;
	bool bind( IpAddress address, uint16_t port ) final override;
	bool connect( const char* hostName, uint16_t port ) final override;
	bool connect( IpAddress address, uint16_t port ) final override;
	void disconnect() final override;

	SocketState state() final override;
	SocketError socketError() const final override;
};