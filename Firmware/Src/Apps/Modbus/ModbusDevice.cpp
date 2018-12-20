#include "ModbusDevice.h"
#include "hal.h"

ModbusDevice::TimerProvider ModbusDevice::timerProvider;

ModbusPotato::system_tick_t ModbusDevice::TimerProvider::ticks() const
{
	return chVTGetSystemTimeX();
}

unsigned long ModbusDevice::TimerProvider::microseconds_per_tick() const
{
	return 1000000 / CH_CFG_ST_FREQUENCY;
}