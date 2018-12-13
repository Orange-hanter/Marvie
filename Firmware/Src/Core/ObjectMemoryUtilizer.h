#pragma once

#include "BaseDynamicThread.h"

using namespace chibios_rt;

class ObjectMemoryUtilizer : private BaseDynamicThread
{
	ObjectMemoryUtilizer();	
	~ObjectMemoryUtilizer();

public:
	static ObjectMemoryUtilizer* instance();	

	void runUtilizer( tprio_t prio );
	void stopUtilizer();

	void utilize( Object* );

private:
	void main();

private:
	static ObjectMemoryUtilizer* inst;
	enum class State { Stopped, Running } state;
	msg_t mboxBuffer[8];
	mailbox_t mailbox;
};