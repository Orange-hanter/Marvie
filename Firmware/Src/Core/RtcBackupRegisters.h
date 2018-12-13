#pragma once

#include "hal.h"

class RtcBackupRegisters
{
public:
	RtcBackupRegisters( uint32_t regsCount, uint32_t offset = 0 );

	bool isValid();
	void setValid();

	inline uint32_t& value( uint32_t index )
	{
		return ( uint32_t& )regs[index];
	}

private:
	volatile uint32_t& crc;
	volatile uint32_t* regs;
	uint32_t regsCount;
};