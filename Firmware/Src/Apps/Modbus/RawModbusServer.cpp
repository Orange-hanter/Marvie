#include "RawModbusServer.h"
#include "Drivers/Interfaces/UsartBasedDevice.h"

RawModbusServer::RawModbusServer( uint32_t stackSize ) : AbstractModbusServer( stackSize )
{
	buffer = nullptr;
	framer = nullptr;
}

RawModbusServer::~RawModbusServer()
{
	stopServer();
	waitForStateChange();
}

void RawModbusServer::setIODevice( IODevice* io )
{
	if( sState == AbstractModbusServer::State::Stopped )
		this->stream.io = io;
}

bool RawModbusServer::startServer( tprio_t prio /*= NORMALPRIO */ )
{
	if( !stream.io )
		return false;
	return AbstractModbusServer::startServer( prio );
}

void RawModbusServer::main()
{
	EventListener listener;
	switch( frameType )
	{
	case FrameType::Rtu:
		buffer = new uint8_t[MODBUS_RTU_DATA_BUFFER_SIZE];
		framer = new ModbusPotato::CModbusRTU( &stream, &timerProvider, buffer, MODBUS_RTU_DATA_BUFFER_SIZE );
		if( stream.io->isSerialDevice() )
			static_cast< ModbusPotato::CModbusRTU* >( framer )->setup( static_cast< UsartBasedDevice* >( stream.io )->baudRate() );
		else
			static_cast< ModbusPotato::CModbusRTU* >( framer )->setup( 115200 ); // dummy value
		break;
	case FrameType::Ascii:
		buffer = new uint8_t[MODBUS_ASCII_DATA_BUFFER_SIZE];
		framer = new ModbusPotato::CModbusASCII( &stream, &timerProvider, buffer, MODBUS_ASCII_DATA_BUFFER_SIZE );
		break;
	case FrameType::Ip:
		buffer = new uint8_t[MODBUS_IP_DATA_BUFFER_SIZE];
		framer = new ModbusPotato::CModbusIP( &stream, &timerProvider, buffer, MODBUS_IP_DATA_BUFFER_SIZE );
		break;
	default:
		break;
	}
	if( buffer == nullptr || framer == nullptr )
		goto End;
	framer->set_handler( &slave );

	stream.io->eventSource().registerMask( &listener, ClientFlag );
	virtual_timer_t timer;
	chVTObjectInit( &timer );
	chEvtAddEvents( InnerEventFlag::ClientFlag );

	while( true )
	{
		eventmask_t em = chEvtWaitAny( ALL_EVENTS );
		if( em & InnerEventFlag::StopRequestFlag )
			break;
		if( em & InnerEventFlag::ClientFlag )
		{
			unsigned int nextInterval = framer->poll();
			if( nextInterval )
				chVTSet( &timer, ( sysinterval_t )nextInterval, timerCallback, chThdGetSelfX() );
			else
				chVTReset( &timer );
		}
	}

	listener.unregister();
	chVTReset( &timer );

End:
	delete buffer;
	delete framer;

	chSysLock();
	sState = State::Stopped;
	eSource.broadcastFlags( ( eventflags_t )EventFlag::StateChanged );
	waitingQueue.dequeueNext( MSG_OK );
}

void RawModbusServer::timerCallback( void* p )
{
	chSysLockFromISR();
	chEvtSignalI( ( thread_t* )p, InnerEventFlag::ClientFlag );
	chSysUnlockFromISR();
}