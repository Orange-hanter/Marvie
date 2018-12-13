#pragma once

#include <stdint.h>

class IpAddress
{
public:
	enum SpecialAddress { Any };

	IpAddress();
	IpAddress( SpecialAddress address );
	IpAddress( uint32_t address );
	IpAddress( uint8_t addr3, uint8_t addr2, uint8_t addr1, uint8_t addr0 );
	IpAddress( const char* address );

	bool operator == ( const IpAddress& b );
	bool operator != ( const IpAddress& b );

	uint8_t addr[4];
};