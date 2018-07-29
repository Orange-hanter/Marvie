#pragma once

#include "UsartBasedDevice.h"
#include "hal.h"

class Usart : public UsartBasedDevice
{
	Usart( SerialDriver* );
	~Usart();

public:
	static Usart* instance( SerialDriver* );
	static bool deleteInstance( SerialDriver* );

	bool open() final override;
	bool open( uint32_t baudRate, DataFormat dataFormat = B8N, StopBits stopBits = StopBits::S1, Mode mode = Mode::RxTx, FlowControl hardwareFlowControl = FlowControl::None ) final override;
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

	uint32_t write( const uint8_t* data, uint32_t size, sysinterval_t timeout ) override;
	uint32_t read( uint8_t* data, uint32_t size, sysinterval_t timeout ) override;
	uint32_t peek( uint32_t pos, uint8_t* data, uint32_t size ) override;
	uint32_t readAvailable() const override;
	bool waitForBytesWritten( sysinterval_t timeout ) final override;
	bool waitForReadAvailable( uint32_t size, sysinterval_t timeout ) final override;

	bool setInputBuffer( uint8_t* buffer, uint32_t size );
	bool setOutputBuffer( uint8_t* buffer, uint32_t size );
	AbstractReadable* inputBuffer() final override;
	AbstractWritable* outputBuffer() final override;
	bool isInputBufferOverflowed() const;
	void resetInputBufferOverflowFlag();

	EvtSource* eventSource();

private:
	static void timerCallback( void* );

private:
	SerialDriver* sd;
	uint32_t _baudRate;
	DataFormat _dataFormat;
	StopBits _stopBits;
	Mode mode;
	FlowControl hardwareFlowControl;
	event_listener_t listener;

	class InputBuffer : public AbstractReadable
	{
	public:
		Usart * usart;

		uint32_t read( uint8_t* data, uint32_t len ) override;
		uint32_t readAvailable() const override;
		bool isOverflowed() const override;
		void resetOverflowFlag() override;
		void clear() override;

		uint8_t& first() override;
		uint8_t& back() override;
		uint8_t& peek( uint32_t index ) override;
		uint32_t peek( uint32_t pos, uint8_t* data, uint32_t len ) override;

		Iterator begin() override;
		Iterator end() override;
		ReverseIterator rbegin() override;
		ReverseIterator rend() override;

	} inBuffer;

	class OutputBuffer : public AbstractWritable
	{
	public:
		Usart* usart;

		uint32_t write( const uint8_t* data, uint32_t len ) override;
		uint32_t write( Iterator begin, Iterator end ) override;
		uint32_t write( Iterator begin, uint32_t size ) override;
		uint32_t writeAvailable() const override;

	} outBuffer;
};