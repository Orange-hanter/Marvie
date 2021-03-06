#include "ff.h"
#include "hal.h"
#include "stm32f4xx_flash.h"
#include "version.h"
#include <string.h>

static FATFS fatFs;
static DIR dir;
static FILINFO fno;
static FIL file;
static uint8_t buffer[512];

extern "C"
{
	void systemHaltHook( const char* reason )
	{
		NVIC_SystemReset();
	}
}

void runApp()
{
	if( *( uint32_t* )0x08008000 != 0xAABBCCDD )
	{
		while( true )
			;
	}

	constexpr uint32_t appStartAddress = 0x08008400;
	volatile void ( *app )( void );
	app = ( volatile void ( * )( void ) )( *( uint32_t* )( appStartAddress + 4 ) );
	SCB->VTOR = appStartAddress;
	__set_MSP( *( ( volatile uint32_t* )appStartAddress ) );
	app();
}

bool openFirmwareFile()
{
	constexpr char signStr[] = "__MARVIE_FIRMWARE__";
	constexpr auto singSize = sizeof( signStr ) - 1;
	if( f_opendir( &dir, "/" ) == FR_OK )
	{
		while( f_readdir( &dir, &fno ) == FR_OK && fno.fname[0] )
		{
			if( ( FF_FS_RPATH && fno.fname[0] == '.' ) || ( fno.fattrib & AM_DIR ) )
				continue;

			auto len = strlen( fno.fname );
			if( len < sizeof( "firmware.bin" ) - 1 || strncmp( fno.fname, "firmware", sizeof( "firmware" ) - 1 ) != 0 )
				continue;
			if( strcmp( fno.fname + len - ( sizeof( ".bin" ) - 1 ), ".bin" ) != 0 )
				continue;
			if( f_open( &file, fno.fname, FA_READ ) != FR_OK )
				break;
			if( f_size( &file ) < singSize + 4 )
			{
				f_close( &file );
				continue;
			}
			UINT br;
			if( f_lseek( &file, f_size( &file ) - singSize ) != FR_OK ||
			    f_read( &file, buffer, singSize, &br ) != FR_OK )
			{
				f_close( &file );
				break;
			}
			if( strncmp( ( const char* )buffer, signStr, singSize ) != 0 )
			{
				f_close( &file );
				continue;
			}
			if( f_lseek( &file, 0 ) != FR_OK )
			{
				f_close( &file );
				break;
			}

			f_closedir( &dir );
			return true;
		}

		f_closedir( &dir );
	}

	return false;
}

int main()
{
	struct BootloaderMetaData
	{
		char version[15 + 1];
	} *metaData = reinterpret_cast< BootloaderMetaData* >( 0x10000000 + 42 * 1024 );
	strcpy( metaData->version, version );

	halInit();
	chSysInit();

	palSetPadMode( GPIOC, 8, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );  // D0
	palSetPadMode( GPIOC, 9, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );  // D1
	palSetPadMode( GPIOC, 10, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST ); // D2
	palSetPadMode( GPIOC, 11, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST ); // D3/CD
	palSetPadMode( GPIOC, 12, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST ); // CLK
	palSetPadMode( GPIOD, 2, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );  // CMD
	sdcStart( &SDCD1, nullptr );

	if( sdcConnect( &SDCD1 ) == HAL_SUCCESS && f_mount( &fatFs, "/", 1 ) == FR_OK && fatFs.fs_type == FM_EXFAT )
	{
		//f_rename( "/_firmware.bin", "/firmware.bin" );
		if( openFirmwareFile() )
		{
			uint32_t totalSize = ( uint32_t )f_size( &file );
			if( totalSize > ( 512 - 33 ) * 1024 )
				goto End;

			FLASH_Unlock();
		Start:
			f_lseek( &file, 0 );
			FLASH_EraseSector( FLASH_Sector_2, VoltageRange_3 ); // 0x08008000-0x0800BFFF (16 кБ) 48KB
			FLASH_EraseSector( FLASH_Sector_3, VoltageRange_3 ); // 0x0800C000-0x0800FFFF (16 кБ) 64KB
			FLASH_EraseSector( FLASH_Sector_4, VoltageRange_3 ); // 0x08010000-0x0801FFFF (64 кБ) 128KB
			FLASH_EraseSector( FLASH_Sector_5, VoltageRange_3 ); // 0x08020000-0x0803FFFF (128 кБ) 256KB
			FLASH_EraseSector( FLASH_Sector_6, VoltageRange_3 ); // 0x08040000-0x0805FFFF (128 кБ) 384KB
			FLASH_EraseSector( FLASH_Sector_7, VoltageRange_3 ); // 0x08060000-0x0807FFFF (128 кБ) 512KB

			uint32_t size = totalSize;
			while( size )
			{
				uint32_t partSize = size;
				if( partSize > 512 )
					partSize = 512;
				else
				{
					for( uint32_t i = partSize; i < ( partSize + 3 ) && i < 512; ++i )
						buffer[i] = 0xFF;
				}

				UINT br;
				if( f_read( &file, buffer, partSize, &br ) != FR_OK || br != partSize )
					NVIC_SystemReset();
				for( uint32_t i = 0; i < partSize; i += 4 )
				{
					FLASH_ProgramWord( 0x08008400 + totalSize - size + i, *( uint32_t* )( buffer + i ) );
					if( *( uint32_t* )( 0x08008400 + totalSize - size + i ) != *( uint32_t* )( buffer + i ) )
						goto Start;
				}
				size -= partSize;
			}

			FLASH_ProgramWord( 0x08008000, 0xAABBCCDD );
			FLASH_Lock();

			f_close( &file );
			f_unlink( fno.fname );
		}
	}
End:
	sdcDisconnect( &SDCD1 );
	sdcStop( &SDCD1 );
	chSysDisable();

	runApp();

	return 0;
}