#pragma once

#include "Flags.h"
#include <cstddef>

class MemoryStatus
{
public:
    MemoryStatus() = delete;

    enum class Region { General = 1, CCM, All = 3 };

    static size_t freeSpace( Flags< Region > regions );
    static size_t usedSpace( Flags< Region > regions );
    static size_t totalSpace( Flags< Region > regions );
};