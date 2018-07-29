#pragma once

#include "RingIterator.h"
#include "ReverseRingIterator.h"

class AbstractReadable
{
public:
	typedef RingIterator< uint8_t > Iterator;
	typedef ReverseRingIterator< uint8_t > ReverseIterator;

	virtual uint32_t read( uint8_t* data, uint32_t len ) = 0;
	virtual uint32_t readAvailable() const = 0;
	virtual bool isOverflowed() const = 0;
	virtual void resetOverflowFlag() = 0;
	virtual void clear() = 0;

	virtual uint8_t& first() = 0;
	virtual uint8_t& back() = 0;
	virtual uint8_t& peek( uint32_t index ) = 0;
	virtual uint32_t peek( uint32_t pos, uint8_t* data, uint32_t len ) = 0;
	inline uint32_t peek( uint8_t* data, uint32_t len ) { return peek( 0, data, len ); }

	virtual Iterator begin() = 0;
	virtual Iterator end() = 0;
	virtual ReverseIterator rbegin() = 0;
	virtual ReverseIterator rend() = 0;
};