#pragma once

#include "cpp_wrappers/ch.hpp"
#include "AbstractReadable.h"
#include "AbstractWritable.h"

using namespace chibios_rt;

class IODevice
{
public:
	virtual ~IODevice() {}

	virtual bool open() = 0;
	virtual bool isOpen() const = 0;
	virtual void reset() = 0;
	virtual void close() = 0;

	virtual int write( const uint8_t* data, uint32_t size, sysinterval_t timeout ) = 0;
	virtual int read( uint8_t* data, uint32_t size, sysinterval_t timeout ) = 0;
	virtual int readAvailable() const = 0;
	virtual bool waitForReadAvailable( uint32_t size, sysinterval_t timeout ) = 0;

	virtual AbstractReadable* inputBuffer() = 0;
	virtual AbstractWritable* outputBuffer() = 0;

	virtual EvtSource* eventSource() = 0;
};