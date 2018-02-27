#include "Usart.h"

static Usart* usarts[8] = {};
int usartId( SerialDriver* sd )
{
#if STM32_SERIAL_USE_USART1
	if( sd == &SD1 )
		return 0;
#endif
#if STM32_SERIAL_USE_USART2
	if( sd == &SD2 )
		return 1;
#endif
#if STM32_SERIAL_USE_USART3
	if( sd == &SD3 )
		return 2;
#endif
#if STM32_SERIAL_USE_UART4
	if( sd == &SD4 )
		return 3;
#endif
#if STM32_SERIAL_USE_UART5
	if( sd == &SD5 )
		return 4;
#endif
#if STM32_SERIAL_USE_USART6
	if( sd == &SD6 )
		return 5;
#endif
#if STM32_SERIAL_USE_UART7
	if( sd == &SD7 )
		return 6;
#endif
#if STM32_SERIAL_USE_UART8
	if( sd == &SD8 )
		return 7;
#endif
}

Usart::Usart( SerialDriver* sd )
{
	baudRate = 115200;
	dataFormat = B8N;
	stopBits = StopBits::S1; 
	mode = Mode::RxTx;
	hardwareFlowControl = FlowControl::None;
	this->sd = sd;
	inBuffer.usart = this;

	sdStop( sd );
	--sd->iqueue.q_size;
	chEvtRegisterMaskWithFlags( &sd->event, &listener, 1, 0 );
	listener.listener = nullptr;
}

Usart::~Usart()
{
	sdStop( sd );
	chEvtUnregister( &sd->event, &listener );

	sd->iqueue.q_size = SERIAL_BUFFERS_SIZE;
	sd->iqueue.q_buffer = sd->ib;
	sd->iqueue.q_rdptr = sd->ib;
	sd->iqueue.q_wrptr = sd->ib;
	sd->iqueue.q_top = sd->ib + SERIAL_BUFFERS_SIZE;

	sd->oqueue.q_size = SERIAL_BUFFERS_SIZE;
	sd->oqueue.q_counter = SERIAL_BUFFERS_SIZE;
	sd->oqueue.q_buffer = sd->ob;
	sd->oqueue.q_rdptr = sd->ob;
	sd->oqueue.q_wrptr = sd->ob;
	sd->oqueue.q_top = sd->ob + SERIAL_BUFFERS_SIZE;
}

Usart* Usart::instance( SerialDriver* sd )
{
	int id = usartId( sd );
	if( usarts[id] )
		return usarts[id];
	return usarts[id] = new Usart( sd );
}

bool Usart::deleteInstance( SerialDriver* sd )
{
	int id = usartId( sd );
	if( usarts[id] )
	{
		delete usarts[id];
		usarts[id] = nullptr;
		return true;
	}

	return false;
}

bool Usart::open()
{
	if( sd->state != sdstate_t::SD_STOP )
		return false;

	SerialConfig config = {};
	config.speed = baudRate;

	if( dataFormat == UsartBasedDevice::B7E )
		config.cr1 = USART_CR1_PCE;
	else if( dataFormat == UsartBasedDevice::B7O )
		config.cr1 = USART_CR1_PCE | USART_CR1_PS;
	else if( dataFormat == UsartBasedDevice::B8E )
		config.cr1 = USART_CR1_M | USART_CR1_PCE;
	else if( dataFormat == UsartBasedDevice::B8O )
		config.cr1 = USART_CR1_M | USART_CR1_PCE | USART_CR1_PS;

	config.cr2 = stopBits << USART_CR2_STOP_Pos;
	
	if( mode == UsartBasedDevice::RxTx )
		config.cr1 |= USART_CR1_RE | USART_CR1_TE;
	else if( mode == UsartBasedDevice::Rx )
		config.cr1 |= USART_CR1_RE;
	else if( mode == UsartBasedDevice::Tx )
		config.cr1 |= USART_CR1_TE;

	if( hardwareFlowControl == UsartBasedDevice::Cts )
		config.cr3 = USART_CR3_CTSE;
	else if( hardwareFlowControl == UsartBasedDevice::Rts )
		config.cr3 = USART_CR3_RTSE;
	else if( hardwareFlowControl == UsartBasedDevice::RtsCts )
		config.cr3 = USART_CR3_CTSE | USART_CR3_RTSE;
	
	sdStart( sd, &config );
	return true;
}

bool Usart::open( uint32_t baudRate, DataFormat dataFormat /*= B8N*/, StopBits stopBits /*= StopBits::S1*/, Mode mode /*= Mode::RxTx*/, FlowControl hardwareFlowControl /*= FlowControl::None */ )
{
	if( sd->state != sdstate_t::SD_STOP )
		return false;

	this->baudRate = baudRate;
	this->dataFormat = dataFormat;
	this->stopBits = stopBits;
	this->mode = mode;
	this->hardwareFlowControl;

	return open();
}

void Usart::setBaudRate( uint32_t baudRate )
{
	if( this->baudRate == baudRate )
		return;
	this->baudRate = baudRate;
	if( sd->state == sdstate_t::SD_STOP )
		return;

	USART_TypeDef* u = sd->usart;
#if STM32_HAS_USART6
	if( ( u == USART1 ) || ( u == USART6 ) )
#else
	if( sdp->usart == USART1 )
#endif
		u->BRR = STM32_PCLK2 / baudRate;
	else
		u->BRR = STM32_PCLK1 / baudRate;
}

void Usart::setDataFormat( DataFormat dataFormat )
{
	if( this->dataFormat == dataFormat )
		return;
	this->dataFormat = dataFormat;
	if( sd->state == sdstate_t::SD_STOP )
		return;

	USART_TypeDef* u = sd->usart;
	u->CR1 &= ~( USART_CR1_M | USART_CR1_PCE | USART_CR1_PS );
	if( dataFormat == UsartBasedDevice::B7E )
		u->CR1 |= USART_CR1_PCE;
	else if( dataFormat == UsartBasedDevice::B7O )
		u->CR1 |= USART_CR1_PCE | USART_CR1_PS;
	else if( dataFormat == UsartBasedDevice::B8E )
		u->CR1 |= USART_CR1_M | USART_CR1_PCE;
	else if( dataFormat == UsartBasedDevice::B8O )
		u->CR1 |= USART_CR1_M | USART_CR1_PCE | USART_CR1_PS;
}

void Usart::setStopBits( StopBits stopBits )
{
	if( this->stopBits == stopBits )
		return;
	this->stopBits = stopBits;
	if( sd->state == sdstate_t::SD_STOP )
		return;

	USART_TypeDef* u = sd->usart;
	u->CR2 &= ~( USART_CR2_STOP_0 | USART_CR2_STOP_1 );
	u->CR2 |= stopBits << USART_CR2_STOP_Pos;
}

bool Usart::isOpen() const
{
	return sd->state == sdstate_t::SD_READY;
}

void Usart::close()
{
	sdStop( sd );
}

USART_TypeDef* Usart::base()
{
	return sd->usart;
}

int Usart::write( uint8_t* data, uint32_t len, sysinterval_t timeout /*= TIME_IMMEDIATE */ )
{
	return oqWriteTimeout( &sd->oqueue, data, len, timeout );
}

int Usart::read( uint8_t* data, uint32_t len, sysinterval_t timeout /*= TIME_IMMEDIATE */ )
{
	return iqReadTimeout( &sd->iqueue, data, len, timeout );
}

int Usart::bytesAvailable() const
{
	return iqGetFullI( &sd->iqueue );
}

bool Usart::setInputBuffer( uint8_t* bp, uint32_t size )
{
	if( sd->state != sdstate_t::SD_STOP )
		return false;

	if( bp )
	{
		sd->iqueue.q_size = size - 1;
		sd->iqueue.q_counter = 0;
		sd->iqueue.q_buffer = bp;
		sd->iqueue.q_rdptr = bp;
		sd->iqueue.q_wrptr = bp;
		sd->iqueue.q_top = bp + size;
	}
	else
	{
		sd->iqueue.q_size = SERIAL_BUFFERS_SIZE - 1;
		sd->iqueue.q_counter = 0;
		sd->iqueue.q_buffer = sd->ib;
		sd->iqueue.q_rdptr = sd->ib;
		sd->iqueue.q_wrptr = sd->ib;
		sd->iqueue.q_top = sd->ib + SERIAL_BUFFERS_SIZE;
	}

	return true;
}

bool Usart::setOutputBuffer( uint8_t* bp, uint32_t size )
{
	if( sd->state != sdstate_t::SD_STOP )
		return false;

	if( bp )
	{
		sd->oqueue.q_size = size;
		sd->oqueue.q_counter = size;
		sd->oqueue.q_buffer = bp;
		sd->oqueue.q_rdptr = bp;
		sd->oqueue.q_wrptr = bp;
		sd->oqueue.q_top = bp + size;
	}
	else
	{
		sd->oqueue.q_size = SERIAL_BUFFERS_SIZE;
		sd->oqueue.q_counter = SERIAL_BUFFERS_SIZE;
		sd->oqueue.q_buffer = sd->ob;
		sd->oqueue.q_rdptr = sd->ob;
		sd->oqueue.q_wrptr = sd->ob;
		sd->oqueue.q_top = sd->ob + SERIAL_BUFFERS_SIZE;
	}

	return true;
}

AbstractByteRingBuffer* Usart::inputBuffer()
{
	return &inBuffer;
}

AbstractByteRingBuffer* Usart::outputBuffer()
{
	return nullptr;
}

bool Usart::isInputBufferOverflowed()
{
	return listener.flags & SD_QUEUE_FULL_ERROR;
}

void Usart::resetInputBufferOverflowFlag()
{
	listener.flags &= ~SD_QUEUE_FULL_ERROR;
}

EvtSource* Usart::eventSource()
{
	return reinterpret_cast< EvtSource* >( &sd->event );
}

uint32_t Usart::InputBuffer::write( uint8_t* data, uint32_t len )
{
	return 0;
}

uint32_t Usart::InputBuffer::write( Iterator begin, Iterator end )
{
	return 0;
}

uint32_t Usart::InputBuffer::read( uint8_t* data, uint32_t len )
{
	chSysLock();
	len = iqReadI( &usart->sd->iqueue, data, len );
	chSysUnlock();

	return len;
}

uint32_t Usart::InputBuffer::writeAvailable()
{
	return 0;
}

uint32_t Usart::InputBuffer::readAvailable()
{
	return iqGetFullI( &usart->sd->iqueue );
}

uint32_t Usart::InputBuffer::size()
{
	return qSizeX( &usart->sd->iqueue );
}

bool Usart::InputBuffer::isBufferOverflowed()
{
	return usart->listener.flags & SD_QUEUE_FULL_ERROR;
}

void Usart::InputBuffer::resetBufferOverflowFlag()
{
	usart->listener.flags &= ~SD_QUEUE_FULL_ERROR;
}

uint8_t& Usart::InputBuffer::first()
{
	return *usart->sd->iqueue.q_rdptr;
}

uint8_t& Usart::InputBuffer::back()
{
	input_queue_t& iq = usart->sd->iqueue;
	if( iq.q_wrptr == iq.q_buffer )
		return iq.q_buffer[iq.q_size];
	return *( iq.q_wrptr - 1 );
}

uint8_t& Usart::InputBuffer::peek( uint32_t index )
{
	input_queue_t& iq = usart->sd->iqueue;
	if( index >= iq.q_size )
		index %= iq.q_size;
	uint8_t* p = iq.q_rdptr + index;
	if( p >= iq.q_top )
		p -= iq.q_size + 1;

	return *p;
}

void Usart::InputBuffer::clear()
{
	chSysLock();
	iqResetI( &usart->sd->iqueue );
	chSysUnlock();
}

AbstractByteRingBuffer::Iterator Usart::InputBuffer::begin()
{
	input_queue_t& iq = usart->sd->iqueue;
	return Iterator( iq.q_buffer, iq.q_rdptr, iq.q_size );
}

AbstractByteRingBuffer::Iterator Usart::InputBuffer::end()
{
	input_queue_t& iq = usart->sd->iqueue;
	return Iterator( iq.q_buffer, iq.q_wrptr, iq.q_size );
}

AbstractByteRingBuffer::ReverseIterator Usart::InputBuffer::rbegin()
{
	input_queue_t& iq = usart->sd->iqueue;
	return ++ReverseIterator( iq.q_buffer, iq.q_wrptr, iq.q_size );
}

AbstractByteRingBuffer::ReverseIterator Usart::InputBuffer::rend()
{
	input_queue_t& iq = usart->sd->iqueue;
	return ++ReverseIterator( iq.q_buffer, iq.q_rdptr, iq.q_size );
}