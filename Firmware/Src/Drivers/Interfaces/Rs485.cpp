#include "Rs485.h"
#include "Core/ServiceEvent.h"
#include "Core/Assert.h"

Rs485::Rs485( Usart* usart, IOPort rePort, IOPort dePort )
{
	this->usart = usart;
	conjugate = false;
	reOutput.attach( rePort );
	deOutput.attach( dePort );
	receiveMode();
}

Rs485::Rs485( Usart* usart, IOPort port )
{
	this->usart = usart;
	conjugate = true;
	reOutput.attach( port );
	receiveMode();
}

bool Rs485::open()
{
	return usart->open();
}

bool Rs485::open( uint32_t baudRate, DataFormat dataFormat /*= B8N*/, StopBits stopBits /*= StopBits::S1*/, Mode mode /*= Mode::RxTx*/, FlowControl hardwareFlowControl /*= FlowControl::None */ )
{
	return usart->open( baudRate, dataFormat, stopBits, mode, hardwareFlowControl );
}

void Rs485::enable()
{
	if( conjugate )
		return;
	receiveMode();
}

void Rs485::disable()
{
	if( conjugate )
		return;
	disableMode();
}

void Rs485::setBaudRate( uint32_t baudRate )
{
	usart->setBaudRate( baudRate );
}

void Rs485::setDataFormat( DataFormat dataFormat )
{
	usart->setDataFormat( dataFormat );
}

void Rs485::setStopBits( StopBits stopBits )
{
	usart->setStopBits( stopBits );
}

uint32_t Rs485::baudRate()
{
	return usart->baudRate();
}

UsartBasedDevice::DataFormat Rs485::dataFormat()
{
	return usart->dataFormat();
}

UsartBasedDevice::StopBits Rs485::stopBits()
{
	return usart->stopBits();
}

bool Rs485::isOpen() const
{
	return usart->isOpen();
}

void Rs485::reset()
{
	usart->reset();
}

void Rs485::close()
{
	usart->close();
}

USART_TypeDef * Rs485::base()
{
	return usart->base();
}

uint32_t Rs485::write( const uint8_t* data, uint32_t size, sysinterval_t timeout )
{
	uint32_t s;
	EventListener listener;
	usart->eventSource().registerMaskWithFlags( &listener, DataServiceEvent, CHN_TRANSMISSION_END );
	chEvtGetAndClearEvents( DataServiceEvent );

	transmitMode();
	tprio_t prio = chThdSetPriority( HIGHPRIO - 1 );
	s = usart->write( data, size, TIME_INFINITE );
	chEvtWaitAny( DataServiceEvent );
	receiveMode();
	chThdSetPriority( prio );

	return s;
}

uint32_t Rs485::read( uint8_t* data, uint32_t size, sysinterval_t timeout )
{
	return usart->read( data, size, timeout );
}

uint32_t Rs485::peek( uint32_t pos, uint8_t* data, uint32_t size )
{
	return usart->peek( pos, data, size );
}

uint32_t Rs485::readAvailable() const
{
	return usart->readAvailable();
}

bool Rs485::waitForBytesWritten( sysinterval_t timeout )
{
	return true;
}

bool Rs485::waitForReadAvailable( uint32_t size, sysinterval_t timeout )
{
	return usart->waitForReadAvailable( size, timeout );
}

AbstractReadable* Rs485::inputBuffer()
{
	return usart->inputBuffer();
}

AbstractWritable* Rs485::outputBuffer()
{
	return nullptr;
}

bool Rs485::isInputBufferOverflowed() const
{
	return usart->isInputBufferOverflowed();
}

void Rs485::resetInputBufferOverflowFlag()
{
	usart->resetInputBufferOverflowFlag();
}

EventSourceRef Rs485::eventSource()
{
	return usart->eventSource();
}

void Rs485::receiveMode()
{
	if( conjugate )
		reOutput.off();
	else
	{
		reOutput.off();
		deOutput.off();
	}
}

void Rs485::transmitMode()
{
	if( conjugate )
		reOutput.on();
	else
	{
		deOutput.on();
		reOutput.on();
	}
}

void Rs485::disableMode()
{
	reOutput.on();
	deOutput.off();
}