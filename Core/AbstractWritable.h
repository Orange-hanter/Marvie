#pragma once

#include "RingIterator.h"
#include "ReverseRingIterator.h"

class AbstractWritable
{
public:
	typedef RingIterator< uint8_t > Iterator;
	typedef ReverseRingIterator< uint8_t > ReverseIterator;

	virtual uint32_t write( const uint8_t* data, uint32_t len ) = 0;
	virtual uint32_t write( Iterator begin, Iterator end ) = 0;
	virtual uint32_t write( Iterator begin, uint32_t size ) = 0;
	virtual uint32_t writeAvailable() const = 0;
};