#include "FileSystem/File.h"
#include "FileSystem/Dir.h"
#include "FileSystem/FileInfo.h"
#include <string>
#include <list>
#include <map>
#include <set>

namespace FileSystemTest
{
	using namespace std;

	int test()
	{
		palSetPadMode( GPIOC, 8, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
		//palSetPadMode( GPIOC, 9, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
		//palSetPadMode( GPIOC, 10, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
		//palSetPadMode( GPIOC, 11, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
		palSetPadMode( GPIOC, 12, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
		palSetPadMode( GPIOD, 2, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
		static SDCConfig sdcConfig;
		sdcConfig.scratchpad = nullptr;
		sdcConfig.bus_width = SDC_MODE_1BIT;
		sdcStart( &SDCD1, &sdcConfig/*nullptr*/ );
		sdcConnect( &SDCD1 );

		FileSystem* fs = new FileSystem;
		assert( fs->mount( "/" ) );
		File* file = new File;

		assert( Dir().mkpath( "/1/2/3/4/5" ) );
		assert( !File::exists( "/1/2/3/4/5/6.txt" ) );
		assert( file->open( "/1/2/3/4/5/6.txt", FileSystem::OpenAppend | FileSystem::Write ) );
		file->close();
		assert( File::exists( "/1/2/3/4/5/6.txt" ) );

		FileInfo info( "/1/2/3/4/5/6.txt" );

		assert( Dir( "/1/2" ).exists() );
		assert( Dir( "/1/2" ).exists( "3" ) );

		Dir().rmpath( "/1/2/3/4/5" );
		Dir( "/1" ).removeRecursively();

		assert( Dir().mkpath( "/1/2/3/4/5" ) );
		assert( Dir().mkpath( "/1/2/a/b/c" ) );
		assert( Dir().mkpath( "/1/2/a/d/e" ) );
		assert( file->open( "/1/2/3/gg1.txt", FileSystem::OpenAlways | FileSystem::Write ) );
		file->write( ( uint8_t* )&file, 42 );
		file->close();
		assert( file->open( "/1/2/3/4/5/gg2.txt", FileSystem::OpenAlways | FileSystem::Write ) );
		file->write( ( uint8_t* )&file, 314 );
		file->close();
		assert( file->open( "/1/2/a/b/c/gg3.txt", FileSystem::OpenAlways | FileSystem::Write ) );
		file->write( ( uint8_t* )&file, 512 );
		file->close();
		assert( file->open( "/1/2/a/d/e/gg4.txt", FileSystem::OpenAlways | FileSystem::Write ) );
		file->write( ( uint8_t* )&file, 666 );
		file->close();
		assert( Dir( "/1" ).contentSize() == 42 + 314 + 512 + 666 );

		auto list = Dir( "/1/2/3/" ).entryList( Dir::Files );
		assert( list.size() == 1 && list.front() == "gg1.txt" );
		list = Dir( "/1/2/3/" ).entryList( Dir::Dirs );
		assert( list.size() == 1 && list.front() == "4" );
		list = Dir( "/1/2/3/" ).entryList( Dir::Dirs | Dir::Files );
		assert( list.size() == 2 && list.front() == "4" && list.back() == "gg1.txt" );

		Dir( "/1" ).removeRecursively();

		fs->unmount();
		delete file;
		delete fs;
		sdcDisconnect( &SDCD1 );
		sdcStop( &SDCD1 );

		return 0;
	}
}