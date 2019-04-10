#pragma once

#include "AbstractTcpServer.h"
#include "TcpSocket.h"
#include "lwip/api.h"

class TcpServer : public AbstractTcpServer
{
public:
	TcpServer();
	~TcpServer();

	bool isListening() const override;
	bool listen( uint16_t port, IpAddress address = IpAddress::Any ) override;
	SocketError serverError() const override;
	void close() override;

	TcpSocket* nextPendingConnection() override;
	bool waitForNewConnection( sysinterval_t timeout = TIME_INFINITE ) override;
	void setNewConnectionsBufferSize( uint32_t inputBufferSize, uint32_t outputBufferSize ) override;

	EvtSource* eventSource() override;

private:
	void recreate();
	void deleteAll();
	void takeNewCon();
	void deleteNewConP();
	void updateState();
	void updateStateS();
	static void netconnCallback( netconn* con, netconn_evt evt, u16_t len );

private:
	netconn* con, *newcon;
	mutable int32_t recvCounter;
	volatile bool prevState;
	EvtSource evtSource;
	static _BinarySemaphore sem;
	static netconn* blockCon;
};