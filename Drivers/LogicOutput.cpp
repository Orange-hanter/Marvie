#include "LogicOutput.h"

void LogicOutput::attach( IOPort port, bool inverse /*= false */ )
{
	this->port = port;
	inv = inverse;

	if( inv )
		palSetPad( port.gpio, port.pinNum );
	else
		palClearPad( port.gpio, port.pinNum );
#ifdef STM32F1
	palSetPadMode( port.gpio, port.pinNum, PAL_MODE_OUTPUT_PUSHPULL );
#else
	palSetPadMode( port.gpio, port.pinNum, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST );
#endif
}

void LogicOutput::on()
{
	if( inv )
		palClearPad( port.gpio, port.pinNum );
	else
		palSetPad( port.gpio, port.pinNum );
}

void LogicOutput::off()
{
	if( inv )
		palSetPad( port.gpio, port.pinNum );
	else
		palClearPad( port.gpio, port.pinNum );
}

void LogicOutput::toggle()
{
	palTogglePad( port.gpio, port.pinNum );
}

void LogicOutput::setState( bool newState )
{
	if( inv )
		newState = !newState;
	if( newState )
		palSetPad( port.gpio, port.pinNum );
	else
		palClearPad( port.gpio, port.pinNum );
}

bool LogicOutput::state()
{
	bool b = palReadLatch( port.gpio ) & ( ( uint16_t )1 << port.pinNum );
	if( inv )
		return !b;
	return b;
}