#include "RawModbusClient.h"
#include "Drivers/Interfaces/UsartBasedDevice.h"
#include "Core/ServiceEvent.h"

RawModbusClient::RawModbusClient() : master( &masterHandler, nullptr, &timerProvider, 1000, 1000 ), masterHandler( this )
{
	frameType = ModbusDevice::FrameType::Rtu;
	buffer = nullptr;
	framer = nullptr;
	reqAddr = 0;
	reqCount = 0;
	reqData = nullptr;
	err = ModbusDevice::Error::NoError;
	waitingResp = false;
}

RawModbusClient::~RawModbusClient()
{
	delete buffer;
	delete framer;
}

void RawModbusClient::setFrameType( FrameType type )
{
	if( buffer && type == frameType )
		return;

	delete buffer;
	delete framer;

	frameType = type;
	switch( frameType )
	{
	case FrameType::Rtu:
		buffer = new uint8_t[MODBUS_RTU_DATA_BUFFER_SIZE];
		framer = new ModbusPotato::CModbusRTU( &stream, &timerProvider, buffer, MODBUS_RTU_DATA_BUFFER_SIZE );
		if( stream.io && stream.io->isSerialDevice() )
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
	
	if( framer )
	{
		framer->set_handler( &master );
		master.set_framer( framer );
	}
}

bool RawModbusClient::readHoldingRegisters( uint8_t slave, uint16_t address, size_t count, uint16_t* data )
{
	if( buffer == nullptr || framer == nullptr )
		return false;
	stream.io->read( nullptr, stream.io->readAvailable(), TIME_INFINITE );
	if( frameType == ModbusDevice::FrameType::Rtu )
		static_cast< ModbusPotato::CModbusRTU* >( framer )->finish_dump_state();
	master.reset_processing_error();

	reqAddr = address;
	reqCount = count;
	reqData = data;
	while( reqCount )
	{
		size_t n = count;
		if( n > 0x007b )
			n = 0x007b;
		if( !master.read_holding_registers_req( slave, reqAddr, n ) || !waitForResponse() )
		{
			reqCount = 0;
			return false;
		}
	}

	reqCount = 0;
	return true;
}

bool RawModbusClient::readInputRegisters( uint8_t slave, uint16_t address, size_t count, uint16_t* data )
{
	if( buffer == nullptr || framer == nullptr )
		return false;
	stream.io->read( nullptr, stream.io->readAvailable(), TIME_INFINITE );
	if( frameType == ModbusDevice::FrameType::Rtu )
		static_cast< ModbusPotato::CModbusRTU* >( framer )->finish_dump_state();
	master.reset_processing_error();

	reqAddr = address;
	reqCount = count;
	reqData = data;
	while( reqCount )
	{
		size_t n = count;
		if( n > 0x007b )
			n = 0x007b;
		if( !master.read_input_registers_req( slave, reqAddr, n ) || !waitForResponse() )
		{
			reqCount = 0;
			return false;
		}
	}

	reqCount = 0;
	return true;
}

bool RawModbusClient::writeSingleRegisters( uint8_t slave, uint16_t address, uint16_t data )
{
	if( buffer == nullptr || framer == nullptr )
		return false;
	stream.io->read( nullptr, stream.io->readAvailable(), TIME_INFINITE );
	if( frameType == ModbusDevice::FrameType::Rtu )
		static_cast< ModbusPotato::CModbusRTU* >( framer )->finish_dump_state();
	master.reset_processing_error();

	reqAddr = address;
	reqCount = 1;
	if( !master.write_single_register_req( slave, reqAddr, data ) || !waitForResponse() )
	{
		reqCount = 0;
		return false;
	}

	reqCount = 0;
	return true;
}

bool RawModbusClient::writeMultipleRegisters( uint8_t slave, uint16_t address, size_t count, const uint16_t* data )
{
	if( buffer == nullptr || framer == nullptr )
		return false;
	stream.io->read( nullptr, stream.io->readAvailable(), TIME_INFINITE );
	if( frameType == ModbusDevice::FrameType::Rtu )
		static_cast< ModbusPotato::CModbusRTU* >( framer )->finish_dump_state();
	master.reset_processing_error();

	reqAddr = address;
	reqCount = count;	
	while( reqCount )
	{
		size_t n = count;
		if( n > 0x007b )
			n = 0x007b;
		if( !master.write_multiple_registers_req( slave, reqAddr, n, data ) || !waitForResponse() )
		{
			reqCount = 0;
			return false;
		}
		data += n;
	}

	reqCount = 0;
	return true;
}

bool RawModbusClient::waitForResponse()
{
	EventListener listener;
	stream.io->eventSource().registerMask( &listener, ServiceEvent::TimeoutServiceEvent );
	virtual_timer_t timer;
	chVTObjectInit( &timer );
	chEvtAddEvents( ServiceEvent::TimeoutServiceEvent );

	err = ModbusDevice::Error::NoError;
	waitingResp = true;
	while( waitingResp )
	{
		chEvtWaitAny( ServiceEvent::TimeoutServiceEvent );
		unsigned int nextInterval = framer->poll();
		unsigned int timeout = master.poll();
		if( nextInterval == 0 )
			nextInterval = timeout;
		if( nextInterval )
			chVTSet( &timer, ( sysinterval_t )nextInterval, timerCallback, chThdGetSelfX() );
	}

	listener.unregister();
	chVTReset( &timer );

	return err == ModbusDevice::Error::NoError;
}

void RawModbusClient::timerCallback( void* p )
{
	chSysLockFromISR();
	chEvtSignalI( ( thread_t* )p, ServiceEvent::TimeoutServiceEvent );
	chSysUnlockFromISR();
}

bool RawModbusClient::MasterHandler::read_holding_registers_rsp( uint16_t address, size_t count, const uint16_t* data )
{
	client->waitingResp = false;
	if( client->reqAddr != address || client->reqCount < count )
	{
		client->err = ModbusDevice::Error::ResponseProcessingError;
		return false;
	}

	client->reqAddr += count;
	client->reqCount -= count;
	for( ; count; --count )
		*( client->reqData++ ) = *( data++ );

	return true;
}

bool RawModbusClient::MasterHandler::read_input_registers_rsp( uint16_t address, size_t count, const uint16_t* data )
{
	return read_holding_registers_rsp( address, count, data );
}

bool RawModbusClient::MasterHandler::write_single_register_rsp( uint16_t address )
{
	client->waitingResp = false;
	if( client->reqAddr != address )
	{
		client->err = ModbusDevice::Error::ResponseProcessingError;
		return false;
	}

	return true;
}

bool RawModbusClient::MasterHandler::write_multiple_registers_rsp( uint16_t address, size_t count )
{
	client->waitingResp = false;
	if( client->reqAddr != address || client->reqCount < count )
	{
		client->err = ModbusDevice::Error::ResponseProcessingError;
		return false;
	}

	client->reqAddr += count;
	client->reqCount -= count;

	return true;
}

bool RawModbusClient::MasterHandler::response_time_out()
{
	client->waitingResp = false;
	client->err = ModbusDevice::Error::TimeoutError;
	return true;
}

bool RawModbusClient::MasterHandler::exception_response( ModbusPotato::modbus_exception_code::modbus_exception_code code )
{
	client->waitingResp = false;
	client->err = ( ModbusDevice::Error )code;
	return true;
}