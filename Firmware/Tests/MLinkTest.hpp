#include "Core/ObjectMemoryUtilizer.h"
#include "Core/Thread.h"
#include "Core/CpuUsageMonitor.h"
#include "Core/Assert.h"
#include "Drivers/Interfaces/Usart.h"
#include "MLinkServer.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>

namespace MLinkTest
{
	static FATFS SDC_FS;
	static uint8_t debugOb[512] = {};
	static MLinkServer* link;
	static uint8_t packetData[255] = {};

	class MLinkCallbacks : public MLinkServer::DataChannelCallback
	{
		bool onOpennig( uint8_t ch, const char* name, uint32_t size ) final override
		{
			assert( ch < 5 );
			char str[6] = "0.dat";
			str[0] = ch + '0';
			assert( strcmp( name, str ) == 0 );
			sprintf( path, "/MLinkTest/%d.dmp", ch );
			if( files[ch] )
			{
				assert( ( err = f_close( files[ch] ) ) == FR_OK );
				assert( ( err = f_unlink( path ) ) == FR_OK );
			}
			else
				files[ch] = new FIL;
			if( ( err = f_open( files[ch], path, FA_OPEN_ALWAYS | FA_WRITE ) ) != FR_OK )
			{
				delete files[ch];
				files[ch] = nullptr;
				assert( false );
			}
			dataSize[ch] = size;

			return true;
		}
		bool newDataReceived( uint8_t ch, const uint8_t* data, uint32_t size ) final override
		{
			assert( ch < 5 );
			assert( files[ch] != nullptr );
			sprintf( path, "/MLinkTest/%d.dmp", ch );
			uint bw;
			assert( ( err = f_write( files[ch], data, size, &bw ) ) == FR_OK );
			assert( bw == size );
			dataSize[ch] -= size;

			return true;
		}
		void onClosing( uint8_t ch, bool canceled ) final override
		{
			assert( ch < 5 );
			assert( files[ch] != nullptr );
			assert( f_close( files[ch] ) == FR_OK );
			sprintf( path, "/MLinkTest/%d.dmp", ch );
			if( canceled )
			{
				f_unlink( path );
				delete files[ch];
				files[ch] = nullptr;
				return;
			}
			sprintf( path + 25, "/MLinkTest/%d.dat", ch );
			delete files[ch];
			files[ch] = nullptr;
			f_unlink( path + 25 );
			assert( ( err = f_rename( path, path + 25 ) ) == FR_OK );
			assert( dataSize[ch] == 0 );
		}

		char path[50] = {};
		FIL* files[5] = {};
		uint32_t dataSize[5] = {};
		FRESULT err;
	} mlinkCallbacks;

	class FileTransmitterThread : private Thread
	{
	public:
		FileTransmitterThread( MLinkServer::DataChannel* channel, FIL* file, tprio_t prio = NORMALPRIO ) :
			Thread( 1024 ), channel( channel ), file( file )
		{
			setPriority( prio );
			start();
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
		MLinkServer::DataChannel* channel;
		FIL* file;
		uint8_t buffer[420];
	};

	class DataXTransmitterThread : private Thread
	{
	public:
		DataXTransmitterThread( MLinkServer::DataChannel* channel, uint32_t size, tprio_t prio = NORMALPRIO ) :
			Thread( 1024 ), channel( channel ), totalSize( size )
		{
			setPriority( prio );
			start();
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
		MLinkServer::DataChannel* channel;
		uint32_t totalSize;
		uint8_t buffer[420];
	};

	class DebugInfoPrinter : private Thread
	{
	public:
		DebugInfoPrinter( Usart* usart, tprio_t prio = NORMALPRIO ) : Thread( 512 ), usart( usart )
		{
			setPriority( prio );
			start();
		}

	private:
		void main()
		{
			EventListener listener;
			CpuUsageMonitor::tryInstance()->eventSource().registerMask( &listener, 1 );
			signalEvents( 1 );
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

	int test()
	{
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

		Usart* debugUsart = Usart::instance( &SD1 );
		debugUsart->setOutputBuffer( debugOb, sizeof( debugOb ) );
		debugUsart->open( 115200, UsartBasedDevice::B8N, UsartBasedDevice::S1, UsartBasedDevice::Tx );
		palSetPadMode( GPIOA, 9, PAL_MODE_ALTERNATE( GPIO_AF_USART1 ) );
		palSetPadMode( GPIOA, 10, PAL_MODE_ALTERNATE( GPIO_AF_USART1 ) );

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
		//link->setIODevice( usart );
		link->setDataChannelCallback( &mlinkCallbacks );
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
					MLinkServer::DataChannel* channel = link->createDataChannel();
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
					MLinkServer::DataChannel* channel = link->createDataChannel();
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
}
