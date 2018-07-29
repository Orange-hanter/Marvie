#pragma once

#include "AbstractSocket.h"
#include "lwip/api.h"

class LwipSocketPrivate
{
	friend class UdpSocket;
	friend class TcpSocket;
	friend class TcpServer;

	LwipSocketPrivate( netconn_type type );
	LwipSocketPrivate( uint32_t );
	~LwipSocketPrivate();

	void setNetconn( netconn* con );
	void recreate();

	bool bind( uint16_t port );
	bool bind( IpAddress address, uint16_t port );
	bool connect( const char* hostName, uint16_t port );
	bool connect( IpAddress address, uint16_t port );
	
	bool isOpen() const;

	SocketState state();
	SocketError socketError() const;
	static SocketError socketError( const netconn* con );

	uint32_t read( uint8_t* data, uint32_t size, sysinterval_t timeout );
	uint32_t netbufPeek( uint32_t pos, uint8_t* data, uint32_t size );
	uint32_t readAvailable() const;	
	bool waitForReadAvailable( uint32_t size, sysinterval_t timeout );

	uint32_t takeNetbuf();
	void deleteFirstPBuf();
	void catNetbuf();
	void updateState();
	void updateStateS();
	static void netconnCallback( netconn* con, netconn_evt evt, u16_t len );

	netconn* con;
	mutable netbuf* lastNetbuf;
	uint32_t offset;
	mutable int32_t recvCounter;
	SocketState prevState;
	threads_queue_t readWaitingQueue;
	EvtSource evtSource;
};