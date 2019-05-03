#include "SharedRs485.h"
#include "Core/ServiceEvent.h"
#include "Core/Assert.h"

SharedRs485::SharedRs485( SharedRs485Control* control, IOPort rePort, IOPort dePort )
{
	this->control = control;
	reOutput.attach( rePort );
	deOutput.attach( dePort );
	disableMode();
	node.value = this;
	_baudRate = 115200;
	_dataFormat = B8N;
	_stopBits = StopBits::S1;
}

SharedRs485::~SharedRs485()
{
	control->remove( this );
}

bool SharedRs485::open()
{
	control->sem.acquire();
	volatile bool res = control->usart->open();
	control->sem.release();

	return res;
}

bool SharedRs485::open( uint32_t baudRate, DataFormat dataFormat /*= B8N*/, StopBits stopBits /*= StopBits::S1*/, Mode mode /*= Mode::RxTx*/, FlowControl hardwareFlowControl /*= FlowControl::None */ )
{
	_baudRate = baudRate;
	_dataFormat = dataFormat;
	_stopBits = stopBits;

	control->sem.acquire();
	volatile bool res = control->usart->open( baudRate, dataFormat, stopBits, mode, hardwareFlowControl );
	control->sem.release();

	return res;
}

void SharedRs485::enable()
{
	chSysLock();
	control->sem.acquire();
	if( control->current )
		control->current->disableMode();
	setActive();
	receiveMode();
	control->sem.release();
	chSchRescheduleS();
	chSysUnlock();
}

void SharedRs485::disable()
{
	chSysLock();
	control->sem.acquire();
	if( control->current == this )
	{
		disableMode();
		control->current = nullptr;
	}
	control->sem.release();
	chSchRescheduleS();
	chSysUnlock();
}

void SharedRs485::setBaudRate( uint32_t baudRate )
{
	_baudRate = baudRate;

	control->sem.acquire();
	control->usart->setBaudRate( baudRate );
	control->sem.release();
}

void SharedRs485::setDataFormat( DataFormat dataFormat )
{
	_dataFormat = dataFormat;

	control->sem.acquire();
	control->usart->setDataFormat( dataFormat );
	control->sem.release();
}

void SharedRs485::setStopBits( StopBits stopBits )
{
	_stopBits = stopBits;

	control->sem.acquire();
	control->usart->setStopBits( stopBits );
	control->sem.release();
}

uint32_t SharedRs485::baudRate()
{
	return _baudRate;
}

UsartBasedDevice::DataFormat SharedRs485::dataFormat()
{
	return _dataFormat;
}

UsartBasedDevice::StopBits SharedRs485::stopBits()
{
	return _stopBits;
}

bool SharedRs485::isOpen() const
{
	return control->usart->isOpen();
}

void SharedRs485::reset()
{
	control->usart->reset();
}

void SharedRs485::close()
{
	control->usart->close();
}

USART_TypeDef* SharedRs485::base()
{
	return control->usart->base();
}

uint32_t SharedRs485::write( const uint8_t* data, uint32_t size, sysinterval_t timeout /*= TIME_INFINITE */ )
{
	uint32_t s;
	EventListener listener;

	control->sem.acquire();
	control->usart->eventSource().registerMaskWithFlags( &listener, DataServiceEvent, CHN_TRANSMISSION_END );
	chEvtGetAndClearEvents( DataServiceEvent );

	if( control->current )
		control->current->disableMode();
	setActive();
	transmitMode();
	tprio_t prio = chThdSetPriority( HIGHPRIO - 1 );
	s = control->usart->write( data, size, TIME_INFINITE );
	chEvtWaitAny( DataServiceEvent );
	receiveMode();
	chThdSetPriority( prio );

	listener.unregister();
	control->sem.release();

	return s;
}

uint32_t SharedRs485::read( uint8_t* data, uint32_t size, sysinterval_t timeout /*= TIME_INFINITE */ )
{
	control->sem.acquire();
	uint32_t n = control->usart->read( data, size, timeout );
	control->sem.release();

	return n;
}

uint32_t SharedRs485::peek( uint32_t pos, uint8_t* data, uint32_t size )
{
	return control->usart->inputBuffer()->peek( pos, data, size );
}

uint32_t SharedRs485::readAvailable() const
{
	return control->usart->readAvailable();
}

bool SharedRs485::waitForBytesWritten( sysinterval_t timeout )
{
	return true;
}

bool SharedRs485::waitForReadAvailable( uint32_t size, sysinterval_t timeout )
{
	return control->usart->waitForReadAvailable( size, timeout );
}

AbstractReadable* SharedRs485::inputBuffer()
{
	return control->usart->inputBuffer();
}

AbstractWritable* SharedRs485::outputBuffer()
{
	return nullptr;
}

bool SharedRs485::isInputBufferOverflowed() const
{
	return control->usart->isInputBufferOverflowed();
}

void SharedRs485::resetInputBufferOverflowFlag()
{
	control->usart->resetInputBufferOverflowFlag();
}

EventSourceRef SharedRs485::eventSource()
{
	return control->usart->eventSource();
}

void SharedRs485::acquireDevice()
{
	control->mutex.lock();
}

void SharedRs485::releaseDevice()
{
	control->mutex.unlock();
}

void SharedRs485::receiveMode()
{
	reOutput.off();
	deOutput.off();
}

void SharedRs485::transmitMode()
{
	deOutput.on();
	reOutput.on();
}

void SharedRs485::disableMode()
{
	reOutput.on();
	deOutput.off();
}

void SharedRs485::setActive()
{
	control->current = this;
	control->usart->setBaudRate( _baudRate );
	control->usart->setDataFormat( _dataFormat );
	control->usart->setStopBits( _stopBits );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

SharedRs485Control::SharedRs485Control( Usart* sharedUsart ) : usart( sharedUsart ), sem( false ), current( nullptr )
{

}

SharedRs485* SharedRs485Control::create( IOPort rePort, IOPort dePort )
{
	SharedRs485* inst = new SharedRs485( this, rePort, dePort );
	chSysLock();
	list.pushBack( &inst->node );
	chSysUnlock();

	return inst;
}

SharedRs485* SharedRs485Control::rs485Shared( uint32_t id )
{
	SharedRs485* inst;
	chSysLock();
	if( list.size() >= id )
		inst = nullptr;
	else
		inst = *( list.begin() + id );
	chSysUnlock();

	return inst;
}

SharedRs485* SharedRs485Control::active()
{
	return current;
}

uint32_t SharedRs485Control::count()
{
	return list.size();
}

void SharedRs485Control::remove( SharedRs485* io )
{
	chSysLock();
	for( auto i = list.begin(); i != list.end(); ++i )
	{
		if( ( *i ) == io )
		{
			list.remove( i );
			break;
		}
	}
	chSysUnlock();
}