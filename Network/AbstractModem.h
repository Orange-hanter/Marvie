#pragma once

#include "AbstractTcpServer.h"
#include "AbstractUdpSocket.h"
#include "IpAddress.h"

enum class ModemStatus { Stopped, Initializing, Working, Stopping };
enum class ModemError { NoError, AuthenticationError };
enum class ModemEvent { Error = 1, StatusChanged = 2 };

class AbstractModem
{
public:
	virtual ~AbstractModem() {};

	virtual void startModem( tprio_t prio ) = 0;
	virtual void stopModem() = 0;
	virtual bool waitForStatusChange( sysinterval_t timeout ) = 0;
	virtual ModemStatus status() = 0;
	virtual ModemError modemError() = 0;
	virtual IpAddress networkAddress() = 0;

	virtual AbstractTcpServer* tcpServer( uint32_t index ) = 0;

	virtual AbstractUdpSocket* createUdpSocket() = 0;
	virtual AbstractUdpSocket* createUdpSocket( uint32_t inputBufferSize, uint32_t outputBufferSize ) = 0;

	virtual AbstractTcpSocket* createTcpSocket() = 0;
	virtual AbstractTcpSocket* createTcpSocket( uint32_t inputBufferSize, uint32_t outputBufferSize ) = 0;
	
	virtual EvtSource* eventSource() = 0;
};