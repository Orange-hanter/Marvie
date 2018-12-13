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