#pragma once

#if defined( DEBUG ) || defined ( USE_ASSERT )
#define assert( p ) { if( !( p ) ) asm( "bkpt 255" ); }
#else
#define assert( p )
#endif