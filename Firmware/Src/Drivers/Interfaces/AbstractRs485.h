#pragma once

#include "Usart.h"

class AbstractRs485 : public UsartBasedDevice
{
public:
	virtual void enable() = 0;
	virtual void disable() = 0;
};