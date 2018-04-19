#pragma once

#include "Network/AbstractTcpServer.h"
#include "SimGsm.h"

class SimGsm;

class SimGsmTcpServer : public AbstractTcpServer
{
	friend class SimGsm;
	SimGsmTcpServer( SimGsm* );
	~SimGsmTcpServer();

public:
	bool isListening() const;
	bool listen( uint16_t port );
	SocketError serverError() const;
	void close();

	AbstractTcpSocket* nextPendingConnection();
	bool waitForNewConnection( sysinterval_t timeout );
	void setNewConnectionsBufferSize( uint32_t inputBufferSize, uint32_t outputBufferSize );

	EvtSource* eventSource();

private:
	void addSocketS( AbstractTcpSocket* );
	void closeHelpS( SocketError error );

private:
	SimGsm * gsm;	
	SocketError sError;
	EvtSource eSource;
	volatile bool listening;
	uint32_t inputBufferSize, outputBufferSize;
	AbstractTcpSocket* root;
	threads_queue_t waitingQueue;
};
