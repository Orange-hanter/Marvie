#pragma once

#include "Object.h"
#include "cpp_wrappers/ch.hpp"

using namespace chibios_rt;

class BaseDynamicThread : public Object, public BaseThread
{
public:
	BaseDynamicThread( uint32_t stackSize, bool tryCCM = true );
	~BaseDynamicThread();

	virtual ThreadReference start( tprio_t prio );

private:
	static void _thd_start( void* );

protected:
	uint8_t* wa;
	uint32_t waSize;
};