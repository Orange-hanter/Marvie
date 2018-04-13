#include "ch.h"

void* operator new( size_t size ) { return chHeapAlloc( nullptr, size ); }
void* operator new[]( size_t size ) { return chHeapAlloc( nullptr, size ); }
void operator delete( void *p ) { if( p ) chHeapFree( p ); }
void operator delete[]( void *p ) { if( p ) chHeapFree( p ); }