#pragma once

#include "AbstractModem.h"

class AbstractGsmModem : public AbstractModem
{
public:
	virtual void setPinCode( uint32_t pinCode ) = 0;
	virtual void setApn( const char* apn ) = 0;
};