#pragma once

#include <stdint.h>

class Crc32SW
{
public:
	static uint32_t crc( const uint8_t* data, uint32_t size, uint32_t crcInit );

private:
	static uint32_t crc32_table[256];
	static uint32_t crc32r_table[256];
	static bool init;
};