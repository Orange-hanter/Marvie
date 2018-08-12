#include "ModbusServer.h"

class DefaulModbusSlaveHandler : public ModbusPotato::ISlaveHandler {} defaultModbusSlaveHandler;

AbstractModbusServer::AbstractModbusServer( uint32_t stackSize ) : BaseDynamicThread( stackSize )
{
	sState = State::Stopped;
	frameType = FrameType::Rtu;
	slaveHandler = &defaultModbusSlaveHandler;
	slave.set_handler( slaveHandler );
	chThdQueueObjectInit( &waitingQueue );
}

AbstractModbusServer::~AbstractModbusServer()
{

}

void AbstractModbusServer::setFrameType( FrameType type )
{
	if( sState != State::Stopped )
		return;
	frameType = type;
}

void AbstractModbusServer::setSlaveHandler( ISlaveHandler* handler )
{
	if( sState != State::Stopped )
		return;
	if( !handler )
		slaveHandler = &defaultModbusSlaveHandler;
	else
		slaveHandler = handler;
	slave.set_handler( slaveHandler );
}

bool AbstractModbusServer::startServer( tprio_t prio /*= NORMALPRIO */ )
{
	if( sState != State::Stopped )
		return false;

	sState = State::Working;
	eSource.broadcastFlags( ( eventflags_t )EventFlag::StateChanged );
	start( prio );

	return true;
}

void AbstractModbusServer::stopServer()
{
	syssts_t sysStatus = chSysGetStatusAndLockX();
	if( sState != State::Working )
	{
		chSysRestoreStatusX( sysStatus );
		return;
	}
	sState = State::Stopping;
	eSource.broadcastFlagsI( ( eventflags_t )EventFlag::StateChanged );
	signalEventsI( InnerEventFlag::StopRequestFlag );
	chSysRestoreStatusX( sysStatus );
}

AbstractModbusServer::State AbstractModbusServer::state()
{
	return sState;
}

bool AbstractModbusServer::waitForStateChange( sysinterval_t timeout /*= TIME_INFINITE */ )
{
	msg_t msg = MSG_OK;
	chSysLock();
	if( sState == State::Stopping )
		msg = chThdEnqueueTimeoutS( &waitingQueue, timeout );
	chSysUnlock();

	return msg == MSG_OK;
}

EvtSource* AbstractModbusServer::eventSource()
{
	return &eSource;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

AbstractModbusNetworkServer::AbstractModbusNetworkServer( uint32_t stackSize ) : AbstractModbusServer( stackSize )
{
	port = 502;
	maxClientsCount = 1;
	clientsCount = 0;
}

void AbstractModbusNetworkServer::setPort( uint16_t port )
{
	if( sState != State::Stopped || !port )
		return;
	this->port = port;
}

void AbstractModbusNetworkServer::setMaxClientsCount( uint32_t count )
{
	if( sState != State::Stopped || !count )
		return;
	if( count > 30 )
		count = 30;
	maxClientsCount = count;
}

uint32_t AbstractModbusNetworkServer::currentClientsCount()
{
	return clientsCount;
}