#pragma once

#include "ModbusMaster.h"
#include "ModbusSlave.h"
#include "ModbusASCII.h"
#include "ModbusRTU.h"
#include "ModbusIP.h"

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