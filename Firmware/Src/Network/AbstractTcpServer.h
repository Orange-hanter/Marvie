#pragma once

#include "AbstractTcpSocket.h"

enum class TcpServerEventFlag { Error = 1, NewConnection = 2 };

class AbstractTcpServer
{
public:
	virtual ~AbstractTcpServer() {};

	virtual bool isListening() const = 0;
	virtual bool listen( uint16_t port, IpAddress address = IpAddress::Any ) = 0;
	virtual SocketError serverError() const = 0;
	virtual void close() = 0;

	virtual AbstractTcpSocket* nextPendingConnection() = 0;
	virtual bool waitForNewConnection( sysinterval_t timeout = TIME_INFINITE ) = 0;
	virtual void setNewConnectionsBufferSize( uint32_t inputBufferSize, uint32_t outputBufferSize ) = 0;

	virtual EventSourceRef eventSource() = 0;
};