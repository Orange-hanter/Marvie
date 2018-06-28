#pragma once

#include "ch.h"

class CCMemoryHeap
{
public:
	static void* alloc( size_t size, unsigned align );
	static inline void* alloc( size_t size )
	{
		return alloc( size, CH_HEAP_ALIGNMENT );
	}
	static void free( void* p );

	// totalp       pointer to a variable that will receive the total fragmented free space or nullptr
	// largestp     pointer to a variable that will receive the largest free free block found space or nullptr
	// return       the number of fragments in the heap.
	static size_t status( size_t* totalp, size_t* largestp );

private:
	static memory_heap_t heap;
};