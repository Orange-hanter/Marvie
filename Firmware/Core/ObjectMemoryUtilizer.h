#pragma once

#include "cpp_wrappers/ch.hpp"

using namespace chibios_rt;

class ObjectMemoryUtilizer : private BaseStaticThread< 128 >
{
	ObjectMemoryUtilizer();	
	~ObjectMemoryUtilizer();

public:
	static ObjectMemoryUtilizer* instance();	

	void runUtilizer( tprio_t prio );
	void stopUtilizer();

	void utilize( void* );

private:
	void main();

private:
	static ObjectMemoryUtilizer* inst;
	enum class State { Stopped, Running } state;
	msg_t mboxBuffer[8];
	mailbox_t mailbox;
};