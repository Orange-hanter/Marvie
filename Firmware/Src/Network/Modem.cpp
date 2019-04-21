#include "Modem.h"
#include "Core/CriticalSectionLocker.h"

Modem::Modem( uint32_t stackSize ) : Thread( stackSize )
{
	mState = ModemState::Stopped;
	mError = ModemError::NoError;
	usart = nullptr;

	mPinCode = 1111;
	mApn = "";
}

Modem::~Modem()
{

}

void Modem::setUsart( Usart* usart )
{
	if( mState != ModemState::Stopped )
		return;

	this->usart = usart;
}

bool Modem::startModem( tprio_t prio /*= NORMALPRIO */ )
{
	if( mState != ModemState::Stopped || !usart )
		return false;

	mError = ModemError::NoError;
	mState = ModemState::Initializing;
	setPriority( prio );
	if( !start() )
	{
		mState = ModemState::Stopped;
		return false;
	}
	extEventSource.broadcastFlags( ( eventflags_t )ModemEvent::StateChanged );
	return true;
}

void Modem::stopModem()
{
	CriticalSectionLocker locker;
	if( mState == ModemState::Stopped || mState == ModemState::Stopping )
		return;
	mState = ModemState::Stopping;
	extEventSource.broadcastFlags( ( eventflags_t )ModemEvent::StateChanged );
	signalEventsI( StopRequestEvent );
}

bool Modem::waitForStateChange( sysinterval_t timeout )
{
	msg_t msg = MSG_OK;
	chSysLock();
	if( mState == ModemState::Initializing || mState == ModemState::Stopping )
		msg = waitingQueue.enqueueSelf( timeout );
	chSysUnlock();

	return msg == MSG_OK;
}

ModemState Modem::state()
{
	return mState;
}

ModemError Modem::error()
{
	return mError;
}

IpAddress Modem::networkAddress()
{
	return netAddress;
}

void Modem::setPinCode( uint32_t pinCode )
{
	this->mPinCode = pinCode;
}

void Modem::setApn( const char* apn )
{
	this->mApn = apn;
}

uint32_t Modem::pinCode()
{
	return mPinCode;
}

const char* Modem::apn()
{
	return mApn;
}

AbstractTcpServer* Modem::tcpServer( uint32_t index )
{
	return nullptr;
}

AbstractUdpSocket* Modem::createUdpSocket()
{
	return nullptr;
}

AbstractUdpSocket* Modem::createUdpSocket( uint32_t inputBufferSize, uint32_t outputBufferSize )
{
	return nullptr;
}

AbstractTcpSocket* Modem::createTcpSocket()
{
	return nullptr;
}

AbstractTcpSocket* Modem::createTcpSocket( uint32_t inputBufferSize, uint32_t outputBufferSize )
{
	return nullptr;
}

EventSourceRef Modem::eventSource()
{
	return &extEventSource;
}

void Modem::setModemStateS( ModemState s )
{
	mState = s;
	extEventSource.broadcastFlags( ( eventflags_t )ModemEvent::StateChanged );
	waitingQueue.dequeueNext( MSG_OK );
}

void Modem::setModemErrorS( ModemError err )
{
	mError = err;
	if( err != ModemError::NoError )
		extEventSource.broadcastFlags( ( eventflags_t )ModemEvent::Error );
}

void Modem::setNetworkAddressS( IpAddress addr )
{
	netAddress = addr;
	extEventSource.broadcastFlags( ( eventflags_t )ModemEvent::NetworkAddressChanged );
}