#include "SharedRs485.h"
#include "Core/Assert.h"

SharedRs485::SharedRs485( SharedRs485Control* control, IOPort rePort, IOPort dePort )
{
	this->control = control;
	reOutput.attach( rePort );
	deOutput.attach( dePort );
	node.value = this;
}

SharedRs485::~SharedRs485()
{
	control->remove( this );
}

bool SharedRs485::open()
{
	control->sem.wait();
	volatile bool res = control->usart->open();
	control->sem.signal();

	return res;
}

bool SharedRs485::open( uint32_t baudRate, DataFormat dataFormat /*= B8N*/, StopBits stopBits /*= StopBits::S1*/, Mode mode /*= Mode::RxTx*/, FlowControl hardwareFlowControl /*= FlowControl::None */ )
{
	control->sem.wait();
	volatile bool res = control->usart->open( baudRate, dataFormat, stopBits, mode, hardwareFlowControl );
	control->sem.signal();

	return res;
}

void SharedRs485::enable()
{
	chSysLock();
	control->sem.waitS();
	if( control->current )
		control->current->disableMode();
	control->current = this;
	receiveMode();
	control->sem.signalI();
	chSchRescheduleS();
	chSysUnlock();
}

void SharedRs485::disable()
{
	chSysLock();
	control->sem.waitS();
	if( control->current == this )
	{
		disableMode();
		control->current = nullptr;
	}
	control->sem.signalI();
	chSchRescheduleS();
	chSysUnlock();
}

void SharedRs485::setBaudRate( uint32_t baudRate )
{
	control->sem.wait();
	control->usart->setBaudRate( baudRate );
	control->sem.signal();
}

void SharedRs485::setDataFormat( DataFormat dataFormat )
{
	control->sem.wait();
	control->usart->setDataFormat( dataFormat );
	control->sem.signal();
}

void SharedRs485::setStopBits( StopBits stopBits )
{
	control->sem.wait();
	control->usart->setStopBits( stopBits );
	control->sem.signal();
}

uint32_t SharedRs485::baudRate()
{
	return control->usart->baudRate();
}

UsartBasedDevice::DataFormat SharedRs485::dataFormat()
{
	return control->usart->dataFormat();
}

UsartBasedDevice::StopBits SharedRs485::stopBits()
{
	return control->usart->stopBits();
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
	assert( timeout == TIME_INFINITE );

	uint32_t s;
	EvtListener listener;

	control->sem.wait();
	control->usart->eventSource()->registerMaskWithFlags( &listener, 1, CHN_TRANSMISSION_END );
	chEvtGetAndClearEvents( 1 );

	if( control->current )
		control->current->disableMode();
	control->current = this;
	transmitMode();
	s = control->usart->write( data, size, TIME_INFINITE );
	chEvtWaitAny( 1 );
	receiveMode();

	control->usart->eventSource()->unregister( &listener );
	control->sem.signal();

	return s;
}

uint32_t SharedRs485::read( uint8_t* data, uint32_t size, sysinterval_t timeout /*= TIME_INFINITE */ )
{
	control->sem.wait();
	uint32_t n = control->usart->read( data, size, timeout );
	control->sem.signal();

	return n;
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

EvtSource* SharedRs485::eventSource()
{
	return control->usart->eventSource();
}

void SharedRs485::acquireDevice()
{
	control->mutex.lock();
}

void SharedRs485::releseDevice()
{
	control->mutex.unlock();
}

void SharedRs485::receiveMode()
{
	reOutput.on();
	deOutput.on();
}

void SharedRs485::transmitMode()
{
	deOutput.off();
	reOutput.off();
}

void SharedRs485::disableMode()
{
	reOutput.on();
	deOutput.off();
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