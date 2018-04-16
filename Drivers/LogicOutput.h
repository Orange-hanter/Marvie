#pragma once

#include "Core/IOPort.h"

class LogicOutput
{
public:
	LogicOutput() { inv = false; }
	~LogicOutput() {}

	void attach( IOPort port, bool inverse = false );
	void on();
	void off();
	void toggle();
	void setState( bool newState ); // On - true, Off - false
	bool state();

private:
	IOPort port;
	bool inv;
};