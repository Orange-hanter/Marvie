#pragma once

#include "RingBuffer.h"
#include "AbstractReadable.h"
#include "AbstractWritable.h"

class BaseByteRingBuffer : public AbstractWritable, public AbstractReadable
{
public:
	typedef RingIterator< uint8_t > Iterator;
	typedef ReverseRingIterator< uint8_t > ReverseIterator;

	BaseByteRingBuffer( uint8_t* buffer, uint32_t size );
	virtual ~BaseByteRingBuffer();

	uint32_t write( const uint8_t* data, uint32_t size, sysinterval_t timeout );
	uint32_t write( const uint8_t* data, uint32_t size ) final override;
	uint32_t write( Iterator begin, Iterator end ) final override;
	uint32_t write( Iterator begin, uint32_t size ) final override;
	uint32_t writeAvailable() const final override;

	uint32_t read( uint8_t* data, uint32_t size, sysinterval_t timeout );
	uint32_t read( uint8_t* data, uint32_t size ) final override;
	uint32_t peek( uint32_t pos, uint8_t* data, uint32_t len ) final override;
	uint32_t readAvailable() const final override;
	bool waitForReadAvailable( uint32_t size, sysinterval_t timeout );
	bool isOverflowed() const final override;
	void resetOverflowFlag() final override;
	void clear() final override;

	uint8_t& first() final override;
	uint8_t& back() final override;
	uint8_t& peek( uint32_t index ) final override;

	Iterator begin() final override;
	Iterator end() final override;
	ReverseIterator rbegin() final override;
	ReverseIterator rend() final override;

private:
	BaseRingBuffer< uint8_t > buffer;
};

template< uint32_t Size >
class StaticByteRingBuffer : public BaseByteRingBuffer
{
public:
	StaticByteRingBuffer() : BaseByteRingBuffer( staticBuffer, Size ) {}

private:
	uint8_t staticBuffer[Size + 1];
};

class DynamicByteRingBuffer : public BaseByteRingBuffer
{
public:
	DynamicByteRingBuffer( uint32_t size ) : BaseByteRingBuffer( dynamicBuffer = new uint8_t[size + 1], size ) {}
	~DynamicByteRingBuffer() { delete dynamicBuffer; }

private:
	uint8_t * dynamicBuffer;
};