#include "Usart.h"
#include "Core/ServiceEvent.h"

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
	return -1;
}

Usart::Usart( SerialDriver* sd )
{
	_baudRate = 115200;
	_dataFormat = B8N;
	_stopBits = StopBits::S1; 
	mode = Mode::RxTx;
	hardwareFlowControl = FlowControl::None;
	this->sd = sd;
	inBuffer.usart = this;
	outBuffer.usart = this;

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
	config.speed = _baudRate;

	if( _dataFormat == UsartBasedDevice::B7E )
		config.cr1 = USART_CR1_PCE;
	else if( _dataFormat == UsartBasedDevice::B7O )
		config.cr1 = USART_CR1_PCE | USART_CR1_PS;
	else if( _dataFormat == UsartBasedDevice::B8E )
		config.cr1 = USART_CR1_M | USART_CR1_PCE;
	else if( _dataFormat == UsartBasedDevice::B8O )
		config.cr1 = USART_CR1_M | USART_CR1_PCE | USART_CR1_PS;

	config.cr2 = _stopBits << USART_CR2_STOP_Pos;
	
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

	this->_baudRate = baudRate;
	this->_dataFormat = dataFormat;
	this->_stopBits = stopBits;
	this->mode = mode;
	this->hardwareFlowControl = hardwareFlowControl;

	return open();
}

void Usart::setBaudRate( uint32_t baudRate )
{
	if( this->_baudRate == baudRate )
		return;
	this->_baudRate = baudRate;
	if( sd->state == sdstate_t::SD_STOP )
		return;

	USART_TypeDef* u = sd->usart;
#if STM32_HAS_USART6
	if( ( u == USART1 ) || ( u == USART6 ) )
#else
	if( u == USART1 )
#endif
		u->BRR = STM32_PCLK2 / baudRate;
	else
		u->BRR = STM32_PCLK1 / baudRate;
}

void Usart::setDataFormat( DataFormat dataFormat )
{
	if( this->_dataFormat == dataFormat )
		return;
	this->_dataFormat = dataFormat;
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
	if( this->_stopBits == stopBits )
		return;
	this->_stopBits = stopBits;
	if( sd->state == sdstate_t::SD_STOP )
		return;

	USART_TypeDef* u = sd->usart;
	u->CR2 &= ~( USART_CR2_STOP_0 | USART_CR2_STOP_1 );
	u->CR2 |= stopBits << USART_CR2_STOP_Pos;
}

uint32_t Usart::baudRate()
{
	return _baudRate;
}

UsartBasedDevice::DataFormat Usart::dataFormat()
{
	return _dataFormat;
}

UsartBasedDevice::StopBits Usart::stopBits()
{
	return _stopBits;
}

bool Usart::isOpen() const
{
	return sd->state == sdstate_t::SD_READY;
}

void Usart::reset()
{
	chSysLock();
	listener.flags &= ~( SD_QUEUE_FULL_ERROR | SD_OVERRUN_ERROR );
	iqResetI( &sd->iqueue );
	oqResetI( &sd->oqueue );
	chSysUnlock();
}

void Usart::close()
{
	sdStop( sd );
}

USART_TypeDef* Usart::base()
{
	return sd->usart;
}

uint32_t Usart::write( const uint8_t* data, uint32_t size, sysinterval_t timeout )
{
	return oqWriteTimeout( &sd->oqueue, data, size, timeout );
}

uint32_t Usart::read( uint8_t* data, uint32_t size, sysinterval_t timeout )
{
	if( size == 0 )
		return 0;
	return iqReadTimeout( &sd->iqueue, data, size, timeout );
}

uint32_t Usart::peek( uint32_t pos, uint8_t* data, uint32_t size )
{
	return inBuffer.peek( pos, data, size );
}

uint32_t Usart::readAvailable() const
{
	return iqGetFullI( &sd->iqueue );
}

bool Usart::waitForBytesWritten( sysinterval_t timeout )
{
	if( !isOpen() )
		return false;
	chSysLock();
	if( oqIsEmptyI( &sd->oqueue ) && sd->usart->SR & USART_SR_TC )
	{
		chSysUnlock();
		return true;
	}
	chSysUnlock();
	if( timeout == TIME_IMMEDIATE )
		return false;

	chEvtGetAndClearEvents( DataServiceEvent | TimeoutServiceEvent );

	virtual_timer_t timer;
	chVTObjectInit( &timer );
	chVTSet( &timer, timeout, timerCallback, chThdGetSelfX() );

	EvtListener listener;
	eventSource()->registerMaskWithFlags( &listener, DataServiceEvent, CHN_TRANSMISSION_END );

	chSysLock();
	if( oqIsEmptyI( &sd->oqueue ) && sd->usart->SR & USART_SR_TC )
	{
		chSysUnlock();
		eventSource()->unregister( &listener );
		chVTReset( &timer );
		return true;
	}
	chSysUnlock();

	eventmask_t em = chEvtWaitAny( DataServiceEvent | TimeoutServiceEvent );
	eventSource()->unregister( &listener );
	chVTReset( &timer );

	if( em & TimeoutServiceEvent )
		return false;
	return true;
}

bool Usart::waitForReadAvailable( uint32_t size, sysinterval_t timeout )
{
	if( !isOpen() )
		return false;
	if( readAvailable() >= size )
		return true;
	if( timeout == TIME_IMMEDIATE )
		return false;

	chEvtGetAndClearEvents( DataServiceEvent | TimeoutServiceEvent );

	virtual_timer_t timer;
	chVTObjectInit( &timer );
	chVTSet( &timer, timeout, timerCallback, chThdGetSelfX() );

	EvtListener usartListener;
	eventSource()->registerMaskWithFlags( &usartListener, DataServiceEvent, CHN_INPUT_AVAILABLE );
	while( readAvailable() < size )
	{
		eventmask_t em = chEvtWaitAny( DataServiceEvent | TimeoutServiceEvent );
		if( em & TimeoutServiceEvent )
			break;
	}

	eventSource()->unregister( &usartListener );
	chVTReset( &timer );

	return readAvailable() >= size;
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

AbstractReadable* Usart::inputBuffer()
{
	return &inBuffer;
}

AbstractWritable* Usart::outputBuffer()
{
	return &outBuffer;
}

bool Usart::isInputBufferOverflowed() const
{
	return listener.flags & SD_QUEUE_FULL_ERROR || listener.flags & SD_OVERRUN_ERROR;
}

void Usart::resetInputBufferOverflowFlag()
{
	listener.flags &= ~( SD_QUEUE_FULL_ERROR | SD_OVERRUN_ERROR );
}

EvtSource* Usart::eventSource()
{
	return reinterpret_cast< EvtSource* >( &sd->event );
}

void Usart::timerCallback( void* p )
{
	chSysLockFromISR();
	chEvtSignalI( reinterpret_cast< thread_t* >( p ), TimeoutServiceEvent );
	chSysUnlockFromISR();
}

uint32_t Usart::InputBuffer::read( uint8_t* data, uint32_t len )
{
	if( len == 0 )
		return 0;

	chSysLock();
	len = iqReadI( &usart->sd->iqueue, data, len );
	chSysUnlock();

	return len;
}

uint32_t Usart::InputBuffer::readAvailable() const
{
	return iqGetFullI( &usart->sd->iqueue );
}

bool Usart::InputBuffer::isOverflowed() const
{
	return usart->listener.flags & SD_QUEUE_FULL_ERROR;
}

void Usart::InputBuffer::resetOverflowFlag()
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

uint32_t Usart::InputBuffer::peek( uint32_t pos, uint8_t* data, uint32_t len )
{
	input_queue_t& iq = usart->sd->iqueue;
	uint32_t n = iqGetFullI( &iq );
	if( n <= pos )
		return 0;
	n -= pos;
	if( len > n )
		len = n;
	else
		n = len;
	auto p = iq.q_rdptr + pos;
	if( p >= iq.q_top )
		p -= iq.q_size + 1;
	while( n )
	{
		*data = *p;
		++data, ++p;
		if( p == iq.q_top )
			p = iq.q_buffer;
		--n;
	}

	return len;
}

void Usart::InputBuffer::clear()
{
	chSysLock();
	iqResetI( &usart->sd->iqueue );
	chSysUnlock();
}

AbstractReadable::Iterator Usart::InputBuffer::begin()
{
	input_queue_t& iq = usart->sd->iqueue;
	return Iterator( iq.q_buffer, iq.q_rdptr, iq.q_size );
}

AbstractReadable::Iterator Usart::InputBuffer::end()
{
	input_queue_t& iq = usart->sd->iqueue;
	return Iterator( iq.q_buffer, iq.q_wrptr, iq.q_size );
}

AbstractReadable::ReverseIterator Usart::InputBuffer::rbegin()
{
	input_queue_t& iq = usart->sd->iqueue;
	return ++ReverseIterator( iq.q_buffer, iq.q_wrptr, iq.q_size );
}

AbstractReadable::ReverseIterator Usart::InputBuffer::rend()
{
	input_queue_t& iq = usart->sd->iqueue;
	return ++ReverseIterator( iq.q_buffer, iq.q_rdptr, iq.q_size );
}

uint32_t Usart::OutputBuffer::write( const uint8_t* data, uint32_t len )
{
	chSysLock();
	len = oqWriteI( &usart->sd->oqueue, data, len );
	chSysUnlock();

	return len;
}

uint32_t Usart::OutputBuffer::write( Iterator begin, Iterator end )
{
	return write( begin, end - begin );
}

uint32_t Usart::OutputBuffer::write( Iterator begin, uint32_t size )
{
	uint32_t done;
	uint8_t* data[2];
	uint32_t dataSize[2];
	ringToLinearArrays( begin, size, data, dataSize, data + 1, dataSize + 1 );
	if( ( done = write( data[0], dataSize[0] ) ) != dataSize[0] )
		return done;
	if( dataSize[1] )
		done += write( data[1], dataSize[1] );

	return done;
}

uint32_t Usart::OutputBuffer::writeAvailable() const
{
	return oqGetEmptyI( &usart->sd->oqueue );
}