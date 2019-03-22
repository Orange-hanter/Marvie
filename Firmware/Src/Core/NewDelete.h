#pragma once
#include <stddef.h>

enum class MemoryAllocPolicy { Default, CCM, TryCCM };
void* operator new( size_t size, MemoryAllocPolicy memPolicy );
void* operator new[]( size_t size, MemoryAllocPolicy memPolicy );
