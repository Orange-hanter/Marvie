#include "NewDelete.h"
#include "ch.h"
#include "CCMemoryHeap.h"

void* operator new( size_t size ) { return chHeapAlloc( nullptr, size ); }
void* operator new[]( size_t size ) { return chHeapAlloc( nullptr, size ); }
//void* operator new( std::size_t size, const NewCCM& ) { return CCMemoryHeap::alloc( size ); }
//void* operator new[]( std::size_t size, const NewCCM& ) { return CCMemoryHeap::alloc( size ); }
//void* operator new( std::size_t size, const TryNewCCM& )
//{
//	void* p = CCMemoryHeap::alloc( size );
//	if( p )
//		return p;
//	return chHeapAlloc( nullptr, size );
//}
//void* operator new[]( std::size_t size, const TryNewCCM& )
//{
//	void* p = CCMemoryHeap::alloc( size );
//	if( p )
//		return p;
//	return chHeapAlloc( nullptr, size );
//}

void operator delete( void *p ) { if( p ) chHeapFree( p ); }
void operator delete[]( void *p ) { if( p ) chHeapFree( p ); }
void operator delete( void *p, unsigned int ) { if( p ) chHeapFree( p ); }
void operator delete[]( void *p, unsigned int ) { if( p ) chHeapFree( p ); }

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