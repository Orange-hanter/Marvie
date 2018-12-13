#include "CCMemoryHeap.h"
#include "CCMemoryAllocator.h"

#define H_NEXT( hp )      ( ( hp )->free.next )
#define H_PAGES( hp )     ( ( hp )->free.pages )

memory_heap_t CCMemoryHeap::heap = []()
{
	memory_heap_t t;
	t.provider = CCMemoryAllocator::alloc;
	H_NEXT( &t.header ) = NULL;
	H_PAGES( &t.header ) = 0;
#if( CH_CFG_USE_MUTEXES == TRUE )
	chMtxObjectInit( &t.mtx );
#else
	chSemObjectInit( &default_heap.sem, ( cnt_t )1 );
#endif

	return t;
}();

void* CCMemoryHeap::alloc( size_t size, unsigned align )
{
	return chHeapAllocAligned( &heap, size, align );
}

void CCMemoryHeap::free( void* p )
{
	chHeapFree( p );
}

size_t CCMemoryHeap::status( size_t* totalp, size_t* largestp )
{
	return chHeapStatus( &heap, totalp, largestp );
}