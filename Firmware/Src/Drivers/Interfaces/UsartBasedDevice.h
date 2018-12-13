#pragma once

#include "Core/IODevice.h"

class UsartBasedDevice : public IODevice
{
public:
	enum DataFormat { B7E, B7O, B8N, B8E, B8O };
	enum StopBits { S1, S0P5, S2, S1P5 };
	enum Mode { Rx, Tx, RxTx };
	enum FlowControl { None, Cts, Rts, RtsCts };

	virtual bool open( uint32_t baudRate, DataFormat dataFormat, StopBits stopBits, Mode mode, FlowControl hardwareFlowControl ) = 0;
	virtual void setBaudRate( uint32_t baudRate ) = 0;
	virtual void setDataFormat( DataFormat dataFormat ) = 0;
	virtual void setStopBits( StopBits stopBits ) = 0;
	virtual uint32_t baudRate() = 0;
	virtual DataFormat dataFormat() = 0;
	virtual StopBits stopBits() = 0;

	virtual USART_TypeDef* base() = 0;

	bool isSerialDevice() final override;
};