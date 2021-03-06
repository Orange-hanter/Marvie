#include "IpAddress.h"
#include "Support/Utility.h"

IpAddress::IpAddress()
{
	addr[0] = addr[1] = addr[2] = addr[3] = 0;
}

IpAddress::IpAddress( uint8_t addr3, uint8_t addr2, uint8_t addr1, uint8_t addr0 )
{
	addr[0] = addr0;
	addr[1] = addr1;
	addr[2] = addr2;
	addr[3] = addr3;
}

IpAddress::IpAddress( uint32_t address )
{
	*( uint32_t* )addr = address;
}

IpAddress::IpAddress( SpecialAddress address )
{
	if( address == Any )
		addr[0] = addr[1] = addr[2] = addr[3] = 0;
}

IpAddress::IpAddress( const char* address )
{
	*( uint32_t* )addr = Utility::parseIp( address );
}

bool IpAddress::operator==( const IpAddress& b )
{
	return *reinterpret_cast< uint32_t* >( addr ) == *reinterpret_cast< const uint32_t* >( b.addr );
}

bool IpAddress::operator!=( const IpAddress& b )
{
	return *reinterpret_cast< uint32_t* >( addr ) != *reinterpret_cast< const uint32_t* >( b.addr );
}