#pragma once

#include "Thread.h"

#define OBJECT_MEMORY_UTILIZER_MAILBOX_SIZE 16

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
	msg_t mboxBuffer[OBJECT_MEMORY_UTILIZER_MAILBOX_SIZE];
	mailbox_t mailbox;
};