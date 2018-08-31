#include "RtcBackupRegisters.h"
#include "Crc32HW.h"

RtcBackupRegisters::RtcBackupRegisters( uint32_t regsCount, uint32_t offset /*= 0 */ ) :
	crc( *( ( volatile uint32_t* )&RTC->BKP0R + offset ) ),
	regs( ( volatile uint32_t* )&RTC->BKP0R + offset + 1 ),
	regsCount( regsCount - 1 )
{

}

bool RtcBackupRegisters::isValid()
{
	Crc32HW::acquire();
	uint32_t currCrc = Crc32HW::crc32Uint( ( const uint32_t* )regs, regsCount, 0xFFFFFFFF );
	Crc32HW::release();

	return crc == currCrc;
}

void RtcBackupRegisters::setValid()
{
	Crc32HW::acquire();
	crc = Crc32HW::crc32Uint( ( const uint32_t* )regs, regsCount, 0xFFFFFFFF );
	Crc32HW::release();
}