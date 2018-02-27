#pragma once

#include "cpp_wrappers/ch.hpp"
#include "AbstractByteRingBuffer.h"

using namespace chibios_rt;

class IODevice
{
public:
	virtual ~IODevice() {}

	virtual int write( uint8_t* data, uint32_t len, sysinterval_t timeout ) = 0;
	virtual int read( uint8_t* data, uint32_t len, sysinterval_t timeout ) = 0;
	virtual int bytesAvailable() const = 0;

	virtual AbstractByteRingBuffer* inputBuffer() = 0;
	virtual AbstractByteRingBuffer* outputBuffer() = 0;

	virtual bool open() = 0;
	virtual bool isOpen() const = 0;
	virtual void close() = 0;

	virtual EvtSource* eventSource() = 0;
};