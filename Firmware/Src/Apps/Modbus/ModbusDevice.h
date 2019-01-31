#pragma once

#include "Lib/modbus/ModbusMaster.h"
#include "Lib/modbus/ModbusSlave.h"
#include "Lib/modbus/ModbusASCII.h"
#include "Lib/modbus/ModbusRTU.h"
#include "Lib/modbus/ModbusIP.h"

class ModbusDevice
{
public:
	enum class FrameType { Rtu, Ascii, Ip };
	enum class Error
	{
		IOError = -3,
		ResponseProcessingError = -2,
		TimeoutError = -1,
		NoError = 0x00,
		IllegalFunctionError = 0x01,
		IllegalDataAddressError = 0x02,
		IllegalDataValueError = 0x03,
		ServerDeviceFailureError = 0x04,
		AcknowledgeError = 0x05,
		ServerDeviceBusyError = 0x06,
		MemoryParityError = 0x08,
		GatewayPathUnavailableError = 0x0A,
		GatewayTargetFailedToRespondError = 0x0B
	};

	virtual ~ModbusDevice() {}
	virtual void setFrameType( FrameType type ) = 0;

protected:
	class TimerProvider : public ModbusPotato::ITimeProvider
	{
		ModbusPotato::system_tick_t ticks() const;
		unsigned long microseconds_per_tick() const;
	};
	static TimerProvider timerProvider;
};