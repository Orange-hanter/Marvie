#include "CCMemoryAllocator.h"
#include "ch.h"

extern uint8_t __ccm_heap_base__[];
extern uint8_t __ccm_heap_end__[];

uint8_t* CCMemoryAllocator::nextMem = __ccm_heap_base__;
uint8_t* CCMemoryAllocator::endMem = __ccm_heap_end__;

void* CCMemoryAllocator::allocI( size_t size, unsigned align, size_t offset )
{
	uint8_t *p, *next;

 	chDbgCheckClassI();
 	chDbgCheck( MEM_IS_VALID_ALIGNMENT( align ) );
 
 	size = MEM_ALIGN_NEXT( size, align );
 	p = ( uint8_t * )MEM_ALIGN_NEXT( nextMem + offset, align );
 	next = p + size;
 
 	/* Considering also the case where there is numeric overflow.*/
 	if( ( next > endMem ) || ( next < nextMem ) )
 		return NULL;
 
 	nextMem = next;

	return p;
}

void* CCMemoryAllocator::alloc( size_t size, unsigned align, size_t offset )
{
	void *p;

	chSysLock();
	p = allocI( size, align, offset );
	chSysUnlock();

	return p;
}

size_t CCMemoryAllocator::status()
{
	return ( size_t )( endMem - nextMem );
}