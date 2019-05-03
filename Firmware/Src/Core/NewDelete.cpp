#include "NewDelete.h"
#include "ch.h"
#include "CCMemoryHeap.h"

void* operator new( size_t size ) { return chHeapAlloc( nullptr, size ); }
void* operator new[]( size_t size ) { return chHeapAlloc( nullptr, size ); }
void* operator new( size_t size, MemoryAllocPolicy memPolicy )
{
	void* p;
	if( memPolicy == MemoryAllocPolicy::TryCCM || memPolicy == MemoryAllocPolicy::CCM )
	{
		p = CCMemoryHeap::alloc( size );
		if( p == nullptr && memPolicy == MemoryAllocPolicy::TryCCM )
			p = chHeapAlloc( nullptr, size );
	}
	else //if( memPolicy == MemoryAllocPolicy::Default )
		p = chHeapAlloc( nullptr, size );
	return p;
}
void* operator new[]( size_t size, MemoryAllocPolicy memPolicy )
{
	void* p;
	if( memPolicy == MemoryAllocPolicy::TryCCM || memPolicy == MemoryAllocPolicy::CCM )
	{
		p = CCMemoryHeap::alloc( size );
		if( p == nullptr && memPolicy == MemoryAllocPolicy::TryCCM )
			p = chHeapAlloc( nullptr, size );
	}
	else //if( memPolicy == MemoryAllocPolicy::Default )
		p = chHeapAlloc( nullptr, size );
	return p;
}

void operator delete( void *p ) { if( p ) chHeapFree( p ); }
void operator delete[]( void *p ) { if( p ) chHeapFree( p ); }
void operator delete( void *p, unsigned int size ) { if( p ) chHeapFree( p ); }
void operator delete[]( void *p, unsigned int size ) { if( p ) chHeapFree( p ); }

extern "C"
{
	void* malloc( size_t nbytes )
	{
		return chHeapAlloc( nullptr, nbytes );
	}
	void* calloc( size_t count, size_t size )
	{
		return chHeapAlloc( nullptr, size * count );
	}
	void free( void* ptr )
	{
		chHeapFree( ptr );
	}
}