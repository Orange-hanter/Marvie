#pragma once

#include "Core/NanoList.h"
#include "Drivers/LogicOutput.h"
#include "Usart.h"

class SharedRs485Control;
class SharedRs485 : public UsartBasedDevice
{
	friend class SharedRs485Control;
	SharedRs485( SharedRs485Control* control, IOPort rePort, IOPort dePort );

public:
	~SharedRs485();

	bool open() final override;
	bool open( uint32_t baudRate, DataFormat dataFormat = B8N, StopBits stopBits = StopBits::S1, Mode mode = Mode::RxTx, FlowControl hardwareFlowControl = FlowControl::None ) final override;
	void enable();
	void disable();
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

	EvtSource* eventSource();
	void acquireDevice();
	void releseDevice();

private:
	inline void receiveMode();
	inline void transmitMode();
	inline void disableMode();

private:
	SharedRs485Control* control;
	LogicOutput reOutput, deOutput;
	NanoList< SharedRs485* >::Node node;
};

class SharedRs485Control
{
	friend class SharedRs485;

public:
	SharedRs485Control( Usart* sharedUsart );

	SharedRs485* create( IOPort rePort, IOPort dePort );
	SharedRs485* rs485Shared( uint32_t id );
	SharedRs485* active();
	uint32_t count();

private:
	void remove( SharedRs485* );

private:
	Usart* usart;
	Mutex mutex;
	BinarySemaphore sem;
	NanoList< SharedRs485* > list;
	SharedRs485* current;
};