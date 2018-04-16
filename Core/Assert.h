#pragma once

#ifdef NDEBUG
#define assert(p)
#else
#define assert(p) {if(!(p)) asm("bkpt 255");}
#endif