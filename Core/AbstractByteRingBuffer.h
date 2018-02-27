#pragma once

#include "stdint.h"
#include "RingIterator.h"
#include "ReverseRingIterator.h"

class AbstractByteRingBuffer
{
public:
	typedef RingIterator< uint8_t > Iterator;
	typedef ReverseRingIterator< uint8_t > ReverseIterator;

	virtual uint32_t write( uint8_t* data, uint32_t len ) = 0;
	virtual uint32_t write( Iterator begin, Iterator end ) = 0;
	virtual uint32_t read( uint8_t* data, uint32_t len ) = 0;
	virtual uint32_t writeAvailable() = 0;
	virtual uint32_t readAvailable() = 0;
	virtual uint32_t size() = 0;

	virtual bool isBufferOverflowed() = 0;
	virtual void resetBufferOverflowFlag() = 0;

	virtual uint8_t& first() = 0;
	virtual uint8_t& back() = 0;
	virtual uint8_t& peek( uint32_t index ) = 0;

	virtual void clear() = 0;

	virtual Iterator begin() = 0;
	virtual Iterator end() = 0;
	virtual ReverseIterator rbegin() = 0;
	virtual ReverseIterator rend() = 0;
};