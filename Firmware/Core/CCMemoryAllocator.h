#pragma once

#include <stdint.h>
#include <stddef.h>

class CCMemoryAllocator
{
public:
	static void* allocI( size_t size, unsigned align, size_t offset );
	static void* alloc( size_t size, unsigned align, size_t offset );
	static size_t status(); // return the size, in bytes, of the free memory

private:
	static uint8_t* nextMem;
	static uint8_t* endMem;
};