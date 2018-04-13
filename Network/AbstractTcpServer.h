#pragma once

#include "AbstractTcpSocket.h"

enum class TcpServerEventFlag { Error = 1, NewConnection = 2 };

class AbstactTcpServer
{
public:
	virtual ~AbstactTcpServer() {};

	virtual bool isListening() const = 0;
	virtual bool listen( uint16_t port ) = 0;
	virtual SocketError serverError() const = 0;
	virtual void close() = 0;

	virtual AbstractTcpSocket* nextPendingConnection() = 0;
	virtual bool waitForNewConnection( sysinterval_t timeout ) = 0;
	virtual void setNewConnectionsBufferSize( uint32_t inputBufferSize, uint32_t outputBufferSize ) = 0;

	virtual EvtSource* eventSource() = 0;
};