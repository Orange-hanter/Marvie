#pragma once

#include "Core/BaseDynamicThread.h"
#include "ModbusDevice.h"

class AbstractModbusServer : public BaseDynamicThread, public ModbusDevice
{
public:
	enum EventFlag : eventflags_t { StateChanged = 1, ClientsCountChanged = 2 };
	enum class State : uint8_t { Stopped, Working, Stopping };
	using ISlaveHandler = ModbusPotato::ISlaveHandler;

	AbstractModbusServer( uint32_t stackSize );
	~AbstractModbusServer();

	void setFrameType( FrameType type ) override;
	void setSlaveHandler( ModbusPotato::ISlaveHandler* handler );

	virtual bool startServer( tprio_t prio = NORMALPRIO );
	virtual void stopServer();
	State state();
	bool waitForStateChange( sysinterval_t timeout = TIME_INFINITE );

	EvtSource* eventSource();

protected:
	enum InnerEventFlag : eventflags_t { StopRequestFlag = 1UL << 31, ServerSocketFlag = 1UL << 30, ClientFlag = 1UL };
	State sState;
	FrameType frameType;
	ModbusPotato::ISlaveHandler* slaveHandler;
	ModbusPotato::CModbusSlave slave;
	EvtSource eSource;
	threads_queue_t waitingQueue;
};

class AbstractModbusNetworkServer : public AbstractModbusServer
{
public:
	AbstractModbusNetworkServer( uint32_t stackSize );

	void setPort( uint16_t port );
	void setMaxClientsCount( uint32_t count );
	uint32_t currentClientsCount();

protected:
	uint16_t port;
	uint16_t maxClientsCount;
	uint16_t clientsCount;
};