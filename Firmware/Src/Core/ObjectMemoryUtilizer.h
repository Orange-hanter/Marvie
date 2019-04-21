#pragma once

#include "Thread.h"

class ObjectMemoryUtilizer : private Thread
{
	ObjectMemoryUtilizer();	
	~ObjectMemoryUtilizer();

public:
	static ObjectMemoryUtilizer* instance();	

	bool runUtilizer( tprio_t prio );
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