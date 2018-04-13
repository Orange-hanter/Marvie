#pragma once

#include "AbstractSocket.h"

class AbstractUdpSocket : public AbstractSocket
{
public:
	virtual bool waitDatagram( sysinterval_t timeout ) = 0;
	virtual	bool hasPendingDatagrams() const = 0;
	virtual uint32_t pendingDatagramSize() const = 0;
	virtual uint32_t readDatagram( uint8_t* data, uint32_t maxSize, IpAddress* address = nullptr, uint16_t* port = nullptr ) = 0;

	virtual int writeDatagram( const uint8_t* data, uint32_t size, IpAddress address, uint16_t port ) = 0;
	virtual int writeDatagram( const uint8_t* data, uint32_t size ) = 0;

	SocketType type() final override;
};