#pragma once

#include "Core/BaseDynamicThread.h"
#include "Drivers/Interfaces/Usart.h"
#include "AbstractTcpServer.h"
#include "AbstractUdpSocket.h"
#include "IpAddress.h"

enum class ModemState { Stopped, Initializing, Working, Stopping };
enum class ModemError { NoError, AuthenticationError, TimeoutError, UnknownError };
enum class ModemEvent { Error = 1, StatusChanged = 2, NetworkAddressChanged = 4 };

class Modem : protected BaseDynamicThread
{
public:
	Modem( uint32_t stackSize );
	virtual ~Modem();

	void setUsart( Usart* );
	virtual bool startModem( tprio_t prio = NORMALPRIO );
	virtual void stopModem();
	bool waitForStateChange( sysinterval_t timeout = TIME_INFINITE );
	ModemState state();
	ModemError error();
	IpAddress networkAddress();

	void setPinCode( uint32_t pinCode );
	void setApn( const char* apn );
	uint32_t pinCode();
	const char* apn();

	virtual AbstractTcpServer* tcpServer( uint32_t index );

	virtual AbstractUdpSocket* createUdpSocket();
	virtual AbstractUdpSocket* createUdpSocket( uint32_t inputBufferSize, uint32_t outputBufferSize );

	virtual AbstractTcpSocket* createTcpSocket();
	virtual AbstractTcpSocket* createTcpSocket( uint32_t inputBufferSize, uint32_t outputBufferSize );
	
	EvtSource* eventSource();

protected:
	void setModemStateS( ModemState s );
	void setModemErrorS( ModemError err );
	void setNetworkAddressS( IpAddress addr );
	inline void setModemState( ModemState s )
	{
		chSysLock();
		setModemStateS( s );
		chSchRescheduleS();
		chSysUnlock();
	}
	inline void setModemError( ModemError err )
	{
		chSysLock();
		setModemErrorS( err );
		chSchRescheduleS();
		chSysUnlock();
	}
	inline void setNetworkAddress( IpAddress addr )
	{
		chSysLock();
		setNetworkAddressS( addr );
		chSchRescheduleS();
		chSysUnlock();
	}

protected:
	enum : eventmask_t { StopRequestEvent = 1 };
	ModemState mState;
	ModemError mError;
	IpAddress netAddress;
	EvtSource extEventSource;
	threads_queue_t waitingQueue;
	Usart* usart;

	uint32_t mPinCode;
	const char* mApn;
};