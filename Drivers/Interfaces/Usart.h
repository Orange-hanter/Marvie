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
	bool isOpen() const final override;
	void close() final override;
	USART_TypeDef* base() final override;

	int write( uint8_t* data, uint32_t len, sysinterval_t timeout = TIME_IMMEDIATE ) final override;
	int read( uint8_t* data, uint32_t len, sysinterval_t timeout = TIME_IMMEDIATE ) final override;
	int bytesAvailable() const final override;

	bool setInputBuffer( uint8_t* buffer, uint32_t size );
	bool setOutputBuffer( uint8_t* buffer, uint32_t size );
	AbstractByteRingBuffer* inputBuffer() final override;
	AbstractByteRingBuffer* outputBuffer() final override;
	bool isInputBufferOverflowed();
	void resetInputBufferOverflowFlag();

	EvtSource* eventSource();

private:
	SerialDriver* sd;
	uint32_t baudRate;
	DataFormat dataFormat;
	StopBits stopBits;
	Mode mode;
	FlowControl hardwareFlowControl;
	event_listener_t listener;

	class InputBuffer : public AbstractByteRingBuffer
	{
	public:
		Usart * usart;

		uint32_t write( uint8_t* data, uint32_t len );
		uint32_t write( Iterator begin, Iterator end );
		uint32_t read( uint8_t* data, uint32_t len );
		uint32_t writeAvailable();
		uint32_t readAvailable();
		uint32_t size();

		bool isBufferOverflowed();
		void resetBufferOverflowFlag();

		uint8_t& first();
		uint8_t& back();
		uint8_t& peek( uint32_t index );

		void clear();

		Iterator begin();
		Iterator end();
		ReverseIterator rbegin();
		ReverseIterator rend();

	} inBuffer;
};