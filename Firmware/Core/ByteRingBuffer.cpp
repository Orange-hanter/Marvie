#include "ByteRingBuffer.h"

BaseByteRingBuffer::BaseByteRingBuffer( uint8_t* buffer, uint32_t size ) : buffer( buffer, size )
{

}

BaseByteRingBuffer::~BaseByteRingBuffer()
{

}

uint32_t BaseByteRingBuffer::write( const uint8_t* data, uint32_t size, sysinterval_t timeout )
{
	return buffer.write( data, size, timeout );
}

uint32_t BaseByteRingBuffer::write( const uint8_t* data, uint32_t size )
{
	return  buffer.write( data, size );
}

uint32_t BaseByteRingBuffer::write( Iterator begin, Iterator end )
{
	return write( begin, end - begin );
}

uint32_t BaseByteRingBuffer::write( Iterator begin, uint32_t size )
{
	uint32_t done;
	uint8_t* data[2];
	uint32_t dataSize[2];
	ringToLinearArrays( begin, size, data, dataSize, data + 1, dataSize + 1 );
	if( ( done = buffer.write( data[0], dataSize[0] ) ) != dataSize[0] )
		return done;
	if( dataSize[1] )
		done += buffer.write( data[1], dataSize[1] );

	return done;
}

uint32_t BaseByteRingBuffer::writeAvailable() const
{
	return buffer.writeAvailable();
}

uint32_t BaseByteRingBuffer::read( uint8_t* data, uint32_t size, sysinterval_t timeout )
{
	return buffer.read( data, size, timeout );
}

uint32_t BaseByteRingBuffer::read( uint8_t* data, uint32_t size )
{
	return  buffer.read( data, size );
}

uint32_t BaseByteRingBuffer::readAvailable() const
{
	return buffer.readAvailable();
}

bool BaseByteRingBuffer::waitForReadAvailable( uint32_t size, sysinterval_t timeout )
{
	return buffer.waitForReadAvailable( size, timeout );
}

bool BaseByteRingBuffer::isOverflowed() const
{
	return buffer.isOverflowed();
}

void BaseByteRingBuffer::resetOverflowFlag()
{
	buffer.resetOverflowFlag();
}

void BaseByteRingBuffer::clear()
{
	buffer.clear();
}

uint8_t& BaseByteRingBuffer::first()
{
	return buffer.first();
}

uint8_t& BaseByteRingBuffer::back()
{
	return buffer.back();
}

uint8_t& BaseByteRingBuffer::peek( uint32_t index )
{
	return buffer.peek( index );
}

BaseByteRingBuffer::Iterator BaseByteRingBuffer::begin()
{
	return buffer.begin();
}

BaseByteRingBuffer::Iterator BaseByteRingBuffer::end()
{
	return buffer.end();
}

BaseByteRingBuffer::ReverseIterator BaseByteRingBuffer::rbegin()
{
	return buffer.rbegin();
}

BaseByteRingBuffer::ReverseIterator BaseByteRingBuffer::rend()
{
	return buffer.rend();
}