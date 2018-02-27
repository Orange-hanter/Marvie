#include "ch.h"

void* operator new( size_t size ) { return chHeapAlloc( nullptr, size ); }
void* operator new[]( size_t size ) { return chHeapAlloc( nullptr, size ); }
void operator delete( void *p ) { chHeapFree( p ); }
void operator delete[]( void *p ) { chHeapFree( p ); }