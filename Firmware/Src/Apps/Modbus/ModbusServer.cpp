#include "ModbusServer.h"
#include "Core/CriticalSectionLocker.h"

class DefaulModbusSlaveHandler : public ModbusPotato::ISlaveHandler {} defaultModbusSlaveHandler;

AbstractModbusServer::AbstractModbusServer( uint32_t stackSize ) : Thread( stackSize )
{
	sState = State::Stopped;
	frameType = FrameType::Rtu;
	slaveHandler = &defaultModbusSlaveHandler;
	slave.set_handler( slaveHandler );
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

	setPriority( prio );
	sState = State::Working;
	if( !start() )
	{
		sState = State::Stopped;
		return false;
	}
	eSource.broadcastFlags( ( eventflags_t )EventFlag::StateChanged );

	return true;
}

void AbstractModbusServer::stopServer()
{
	CriticalSectionLocker locker;
	if( sState != State::Working )
		return;
	sState = State::Stopping;
	eSource.broadcastFlags( ( eventflags_t )EventFlag::StateChanged );
	signalEventsI( InnerEventFlag::StopRequestFlag );
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
		msg = waitingQueue.enqueueSelf( timeout );
	chSysUnlock();

	return msg == MSG_OK;
}

EventSourceRef AbstractModbusServer::eventSource()
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