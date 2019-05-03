#pragma once

#include "ch.h"

class CriticalSectionLocker
{
	volatile const syssts_t syssts;

public:
	inline CriticalSectionLocker() : syssts( chSysGetStatusAndLockX() ) {}

	inline ~CriticalSectionLocker()
	{
		chSysRestoreStatusX( syssts );
	}
};