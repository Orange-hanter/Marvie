#include "SimGsmTcpServer.h"

SimGsmTcpServer::SimGsmTcpServer( SimGsm* gsm )
{
	this->gsm = gsm;
	sError = SocketError::NoError;
	listening = false;
	inputBufferSize = 16, outputBufferSize = 0;
	root = nullptr;
	chThdQueueObjectInit( &waitingQueue );
}

SimGsmTcpServer::~SimGsmTcpServer()
{
	chSysLock();
	while( root )
	{
		AbstractTcpSocket* socket = root;
		root = reinterpret_cast< AbstractTcpSocket* >( root->userData );
		chSysUnlock();
		delete socket;
		chSysLock();
	}
	chThdDequeueNextI( &waitingQueue, MSG_RESET );
	chSysUnlock();
}

bool SimGsmTcpServer::isListening() const
{
	return listening;
}

bool SimGsmTcpServer::listen( uint16_t port )
{
	if( listening )
		return false;
	return gsm->serverStart( port );
}

SocketError SimGsmTcpServer::serverError() const
{
	return sError;
}

void SimGsmTcpServer::close()
{
	gsm->serverStop();
}

AbstractTcpSocket* SimGsmTcpServer::nextPendingConnection()
{
	AbstractTcpSocket* socket;
	chSysLock();
	socket = root;
	if( root )
	{
		root = reinterpret_cast< AbstractTcpSocket* >( root->userData );
		socket->userData = nullptr;
	}
	chSysUnlock();

	return socket;
}

bool SimGsmTcpServer::waitForNewConnection( sysinterval_t timeout )
{
	chSysLock();
	if( root )
	{
		chSysUnlock();
		return true;
	}
	msg_t msg = chThdEnqueueTimeoutS( &waitingQueue, timeout );
	chSysUnlock();

	return msg == MSG_OK;
}

void SimGsmTcpServer::setNewConnectionsBufferSize( uint32_t inputBufferSize, uint32_t outputBufferSize )
{
	this->inputBufferSize = inputBufferSize;
	this->outputBufferSize = outputBufferSize;
}

EvtSource* SimGsmTcpServer::eventSource()
{
	return &eSource;
}

void SimGsmTcpServer::addSocketS( AbstractTcpSocket* socket )
{
	if( !root )
		eSource.broadcastFlagsI( ( eventflags_t )TcpServerEventFlag::NewConnection );
	socket->userData = root;
	root = socket;
	chThdDequeueNextI( &waitingQueue, MSG_OK );
}

void SimGsmTcpServer::closeHelpS( SocketError error )
{
	listening = false;
	sError = error;
	if( sError != SocketError::NoError )
		eSource.broadcastFlagsI( ( eventflags_t )TcpServerEventFlag::Error );
}