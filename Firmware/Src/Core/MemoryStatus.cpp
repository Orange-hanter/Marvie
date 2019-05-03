#include "MemoryStatus.h"
#include "CCMemoryAllocator.h"
#include "CCMemoryHeap.h"
#include "ch.h"

extern uint8_t __ram0_start__[];
extern uint8_t __ram0_end__[];
extern uint8_t __ram4_start__[];
extern uint8_t __ram4_end__[];

size_t MemoryStatus::freeSpace( Flags< Region > regions )
{
	size_t mem = 0;
	if( regions.testFlag( Region::General ) )
	{
		mem += chCoreGetStatusX();
		size_t total;
		mem += chHeapStatus( nullptr, &total, nullptr ) * 8;
		mem += total;
	}
	if( regions.testFlag( Region::CCM ) )
	{
		mem += CCMemoryAllocator::status();
		size_t total;
		mem += CCMemoryHeap::status( &total, nullptr ) * 8;
		mem += total;
	}

	return mem;
}

size_t MemoryStatus::usedSpace( Flags< Region > regions )
{
	return totalSpace( regions ) - freeSpace( regions );
}

size_t MemoryStatus::totalSpace( Flags< Region > regions )
{
    size_t total = 0;
	if( regions.testFlag( Region::General ) )
		total += __ram0_end__ - __ram0_start__;
	if( regions.testFlag( Region::CCM ) )
		total += __ram4_end__ - __ram4_start__;

	return total;
}