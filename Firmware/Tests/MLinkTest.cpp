#include "Core/ObjectMemoryUtilizer.h"
#include "Core/BaseDynamicThread.h"
#include "Core/CpuUsageMonitor.h"
#include "Core/Assert.h"
#include "Drivers/Interfaces/Usart.h"
#include "MLinkServer.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>

static FATFS SDC_FS;
static uint8_t ib[1024] = {}, ob[1024] = {};
static uint8_t debugOb[512] = {};
static MLinkServer* link;
static uint8_t packetData[255];

class MLinkCallbacks : public MLinkServer::ComplexDataCallback
{
	uint32_t onOpennig( uint8_t id, const char* name, uint32_t size ) final override
	{
		assert( id < 5 );
		char str[6] = "0.dat";
		str[0] = id + '0';
		assert( strcmp( name, str ) == 0 );
		sprintf( path, "/MLinkTest/%d.dmp", id );
		if( files[id] )
		{
			assert( ( err = f_close( files[id] ) ) == FR_OK );
			assert( ( err = f_unlink( path ) ) == FR_OK );
		}
		else
			files[id] = new FIL;
		if( ( err = f_open( files[id], path, FA_OPEN_ALWAYS | FA_WRITE ) ) != FR_OK )
		{
			delete files[id];
			files[id] = nullptr;
			assert( false );
		}
		dataSize[id] = size;

		return 1;
	}
	bool newDataReceived( uint8_t id, const uint8_t* data, uint32_t size ) final override
	{
		assert( id < 5 );
		assert( files[id] != nullptr );
		sprintf( path, "/MLinkTest/%d.dmp", id );
		uint bw;
		assert( ( err = f_write( files[id], data, size, &bw ) ) == FR_OK );
		assert( bw == size );
		dataSize[id] -= size;

		return true;
	}
	void onClosing( uint8_t id, bool canceled ) final override
	{
		assert( id < 5 );
		assert( files[id] != nullptr );
		assert( f_close( files[id] ) == FR_OK );
		sprintf( path, "/MLinkTest/%d.dmp", id );
		if( canceled )
		{
			f_unlink( path );
			delete files[id];
			files[id] = nullptr;
			return;
		}
		sprintf( path + 25, "/MLinkTest/%d.dat", id );
		delete files[id];
		files[id] = nullptr;
		f_unlink( path + 25 );
		assert( ( err = f_rename( path, path + 25 ) ) == FR_OK );
		assert( dataSize[id] == 0 );
	}

	char path[50] = {};
	FIL* files[5] = {};
	uint32_t dataSize[5] = {};
	FRESULT err;
} mlinkCallbacks;

class FileTransmitterThread : private BaseDynamicThread
{
public:
	FileTransmitterThread( MLinkServer::ComplexDataChannel* channel, FIL* file, tprio_t prio = NORMALPRIO ) :
		BaseDynamicThread( 512 ), channel( channel ), file( file )
	{
		start( prio );
	}

private:
	void main()
	{
		while( true )
		{
			uint size;
			FRESULT err;
			assert( ( err = f_read( file, buffer, 420, &size ) ) == FR_OK );
			if( size == 0 )
				break;
			if( !channel->sendData( buffer, size ) )
				break;
		}
		channel->close();
		f_close( file );
		delete channel;
		delete file;

		chSysLock();
		deleteLater();
		exitS( 0 );
	}

private:
	MLinkServer::ComplexDataChannel* channel;
	FIL* file;
	uint8_t buffer[420];
};

class DataXTransmitterThread : private BaseDynamicThread
{
public:
	DataXTransmitterThread( MLinkServer::ComplexDataChannel* channel, uint32_t size, tprio_t prio = NORMALPRIO ) :
		BaseDynamicThread( 512 ), channel( channel ), totalSize( size )
	{
		start( prio );
	}

private:
	void main()
	{
		uint32_t i = 0;
		while( totalSize )
		{
			uint size;
			for( size = 0; size < totalSize && size < 420; ++size )
				buffer[size] = '0' + i++ % 10;
			totalSize -= size;
			if( !channel->sendData( buffer, size ) )
				break;
		}
		channel->close();
		delete channel;

		chSysLock();
		deleteLater();
		exitS( 0 );
	}

private:
	MLinkServer::ComplexDataChannel* channel;	
	uint32_t totalSize;
	uint8_t buffer[420];
};

class DebugInfoPrinter : private BaseDynamicThread
{
public:
	DebugInfoPrinter( Usart* usart, tprio_t prio = NORMALPRIO ) : BaseDynamicThread( 512 ), usart( usart )
	{
		start( prio );
	}

private:
	void main()
	{
		EvtListener listener;
		CpuUsageMonitor::tryInstance()->eventSource()->registerMask( &listener, 1 );
		chEvtSignal( thread_ref, 1 );
		while( true )
		{
			chEvtWaitAny( ALL_EVENTS );
			sprintf( str, "CPU usage = %d\n", ( int )CpuUsageMonitor::tryInstance()->usage() );
			usart->write( ( uint8_t* )str, strlen( str ), TIME_INFINITE );
		}
	}

private:
	Usart * usart;
	char str[50];
};

int main()
{
	halInit();
	chSysInit();

	CpuUsageMonitor::instance();

	palSetPadMode( GPIOC, 8, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
	palSetPadMode( GPIOC, 9, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
	palSetPadMode( GPIOC, 10, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
	palSetPadMode( GPIOC, 11, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
	palSetPadMode( GPIOC, 12, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
	palSetPadMode( GPIOD, 2, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
	sdcStart( &SDCD1, NULL );

	if( !blkIsInserted( &SDCD1 ) )
		chSysHalt( "SD card not inserted" );
	if( sdcConnect( &SDCD1 ) != HAL_SUCCESS || f_mount( &SDC_FS, "/", 1 ) != FR_OK )
		chSysHalt( "SD card initialization error" );
	FRESULT err = f_unlink( "/MLinkTest" );
	err = f_mkdir( "/MLinkTest" );

	Usart* usart = Usart::instance( &SD1 );
	usart->setOutputBuffer( ob, sizeof( ob ) );
	usart->setInputBuffer( ib, sizeof( ib ) );
	usart->open();
	palSetPadMode( GPIOA, 10, PAL_MODE_ALTERNATE( GPIO_AF_USART1 ) );
	palSetPadMode( GPIOA, 9, PAL_MODE_ALTERNATE( GPIO_AF_USART1 ) );

	Usart* debugUsart = Usart::instance( &SD2 );
	debugUsart->setOutputBuffer( debugOb, sizeof( debugOb ) );
	debugUsart->open( 115200, UsartBasedDevice::B8N, UsartBasedDevice::S1, UsartBasedDevice::Tx );
	palSetPadMode( GPIOA, 2, PAL_MODE_ALTERNATE( GPIO_AF_USART2 ) );
	palSetPadMode( GPIOA, 3, PAL_MODE_ALTERNATE( GPIO_AF_USART2 ) );

	crcStart( &CRCD1, nullptr );
	
	/*{
		const uint8_t* ddd = ( const uint8_t* )"123456789abcdefghijklmno";
		for( int iLen = 2; iLen <= 24; ++iLen )
		{
			uint32_t crc = Crc32HW::crc32Byte( ddd, iLen, -1 );
			for( int i = 1; i < iLen; ++i )
				assert( Crc32HW::crc32Byte( ddd, i, ddd + i, iLen - i ) == crc );
		}
	}*/

	ObjectMemoryUtilizer::instance()->runUtilizer( NORMALPRIO );
	new DebugInfoPrinter( debugUsart, NORMALPRIO - 1 );

	link = new MLinkServer;
	link->setIODevice( usart );
	link->setComplexDataReceiveCallback( &mlinkCallbacks );
	link->startListening( NORMALPRIO + 1 );

	enum PacketType { Hello, GetData, GetDataX };
	while( true )
	{
		if( !link->waitPacket() )
		{
			link->waitForStateChanged();
			link->confirmSession();
			continue;
		}
		uint8_t type;
		uint32_t size = link->readPacket( &type, packetData, 255 );
		if( type != 255 )
		{
			if( type == Hello )
			{
				assert( strcmp( ( const char* )packetData, "Hello" ) == 0 );
				link->sendPacket( Hello, ( const uint8_t* )"Hello", 6 );
			}
			else if( type == GetData )
			{
				assert( size == 1 && packetData[0] < 5 );
				FIL* file = new FIL;
				char path[] = "/MLinkTest/0.dat";
				path[11] = packetData[0] + '0';
				assert( f_open( file, path, FA_OPEN_EXISTING | FA_READ ) == FR_OK );
				MLinkServer::ComplexDataChannel* channel = link->createComplexDataChannel();
				char name[] = "0.dat";
				name[0] = packetData[0] + '0';
				if( !channel->open( packetData[0], name, f_size( file ) ) )
				{
					f_close( file );
					delete file;
					delete channel;
				}
				else
					new FileTransmitterThread( channel, file );
			}
			else if( type == GetDataX )
			{
				MLinkServer::ComplexDataChannel* channel = link->createComplexDataChannel();
				char name[] = "3.dat";
				uint32_t size = *( uint32_t* )packetData;
				if( !channel->open( 3, name, size ) )
					delete channel;
				else
					new DataXTransmitterThread( channel, size );
			}
		}
	}

	return 0;
}