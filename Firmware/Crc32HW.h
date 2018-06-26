#pragma once

#include "hal.h"

class Crc32HW
{
public:
	static uint32_t crc32Uint( const uint32_t* pData, size_t count, uint32_t init );
	static uint32_t crc32Byte( const uint8_t* pData, size_t count, uint32_t init );
	static uint32_t crc32Byte( const uint8_t* pDataA, uint32_t sizeA, const uint8_t* pDataB, uint32_t sizeB );
};