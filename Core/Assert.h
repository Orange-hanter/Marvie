#pragma once

#ifdef NDEBUG
#define assert(p) (p)
#else
#define assert(p) {if(!(p)) asm("bkpt 255");}
#endif