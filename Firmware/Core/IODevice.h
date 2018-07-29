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

	virtual uint32_t write( const uint8_t* data, uint32_t size, sysinterval_t timeout = TIME_INFINITE ) = 0;
	virtual uint32_t read( uint8_t* data, uint32_t size, sysinterval_t timeout = TIME_INFINITE ) = 0;
	virtual uint32_t peek(uint32_t pos, uint8_t* data, uint32_t size ) = 0;
	inline uint32_t peek( uint8_t* data, uint32_t size ) { return peek( 0, data, size ); }
	virtual uint32_t readAvailable() const = 0;
	virtual bool waitForBytesWritten( sysinterval_t timeout ) = 0;
	virtual bool waitForReadAvailable( uint32_t size, sysinterval_t timeout ) = 0;

	virtual AbstractReadable* inputBuffer() = 0;
	virtual AbstractWritable* outputBuffer() = 0;

	virtual void acquireDevice() {}
	virtual void releseDevice() {}
	virtual EvtSource* eventSource() = 0;
};