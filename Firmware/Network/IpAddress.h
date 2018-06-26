#pragma once

#include <stdint.h>

class IpAddress
{
public:
	IpAddress();
	IpAddress( uint8_t addr3, uint8_t addr2, uint8_t addr1, uint8_t addr0 );

	bool operator == ( const IpAddress& b );
	bool operator != ( const IpAddress& b );

	uint8_t addr[4];
};