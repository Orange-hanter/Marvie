#pragma once

#include "Drivers/LogicOutput.h"
#include "AbstractRs485.h"

class Rs485 : public AbstractRs485
{
public:
	Rs485( Usart* usart, IOPort rePort, IOPort dePort );
	Rs485( Usart* usart, IOPort port );

	bool open() final override;
	bool open( uint32_t baudRate, DataFormat dataFormat = B8N, StopBits stopBits = StopBits::S1, Mode mode = Mode::RxTx, FlowControl hardwareFlowControl = FlowControl::None ) final override;
	void enable() override;
	void disable() override;
	void setBaudRate( uint32_t baudRate ) final override;
	void setDataFormat( DataFormat dataFormat ) final override;
	void setStopBits( StopBits stopBits ) final override;
	uint32_t baudRate() final override;
	DataFormat dataFormat() final override;
	StopBits stopBits() final override;
	bool isOpen() const final override;
	void reset() final override;
	void close() final override;
	USART_TypeDef* base() final override;

	uint32_t write( const uint8_t* data, uint32_t size, sysinterval_t timeout = TIME_INFINITE ) override;
	uint32_t read( uint8_t* data, uint32_t size, sysinterval_t timeout = TIME_INFINITE ) override;
	uint32_t peek( uint32_t pos, uint8_t* data, uint32_t size ) override;
	uint32_t readAvailable() const override;
	bool waitForBytesWritten( sysinterval_t timeout ) final override;
	bool waitForReadAvailable( uint32_t size, sysinterval_t timeout ) final override;

	AbstractReadable* inputBuffer() final override;
	AbstractWritable* outputBuffer() final override;
	bool isInputBufferOverflowed() const;
	void resetInputBufferOverflowFlag();

	EventSourceRef eventSource();

private:
	inline void receiveMode();
	inline void transmitMode();
	inline void disableMode();

private:
	Usart* usart;
	volatile bool conjugate;
	LogicOutput reOutput;
	LogicOutput deOutput;
};