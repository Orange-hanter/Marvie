#include "Modem.h"

Modem::Modem( uint32_t stackSize ) : BaseDynamicThread( stackSize )
{
	mState = ModemState::Stopped;
	mError = ModemError::NoError;
	chThdQueueObjectInit( &waitingQueue );
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
	extEventSource.broadcastFlags( ( eventflags_t )ModemEvent::StateChanged );
	start( prio );
	return true;
}

void Modem::stopModem()
{
	chSysLock();
	if( mState == ModemState::Stopped || mState == ModemState::Stopping )
	{
		chSysUnlock();
		return;
	}
	mState = ModemState::Stopping;
	extEventSource.broadcastFlagsI( ( eventflags_t )ModemEvent::StateChanged );
	chEvtSignalI( thread_ref, StopRequestEvent );
	chSchRescheduleS();
	chSysUnlock();
}

bool Modem::waitForStateChange( sysinterval_t timeout )
{
	msg_t msg = MSG_OK;
	chSysLock();
	if( mState == ModemState::Initializing || mState == ModemState::Stopping )
		msg = chThdEnqueueTimeoutS( &waitingQueue, timeout );
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

EvtSource* Modem::eventSource()
{
	return &extEventSource;
}

void Modem::setModemStateS( ModemState s )
{
	mState = s;
	extEventSource.broadcastFlagsI( ( eventflags_t )ModemEvent::StateChanged );
	chThdDequeueNextI( &waitingQueue, MSG_OK );
}

void Modem::setModemErrorS( ModemError err )
{
	mError = err;
	if( err != ModemError::NoError )
		extEventSource.broadcastFlagsI( ( eventflags_t )ModemEvent::Error );
}

void Modem::setNetworkAddressS( IpAddress addr )
{
	netAddress = addr;
	extEventSource.broadcastFlagsI( ( eventflags_t )ModemEvent::NetworkAddressChanged );
}