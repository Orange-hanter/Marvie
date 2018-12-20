#include "FileSystem/FileSystem.h"
#include "Log/MarvieLog.h"
#include "Core/Assert.h"
#include "hal.h"
#include "Crc32HW.h"
#include <math.h>

namespace MarvieLogSystemTest
{
	using namespace std;

	class SignalProviderA : public AbstractSRSensor::SignalProvider
	{
		DateTime dateTime = DateTimeService::currentDateTime();
		int num = 0;
	public:
		float analogSignal( uint32_t block, uint32_t line )
		{
			return block * 100 + line;
		}
		bool digitSignal( uint32_t block, uint32_t line )
		{
			return false;
		}
		uint32_t digitSignals( uint32_t block )
		{
			if( block == 0 )
				return block + 1;
			if( dateTime.msecsTo( DateTimeService::currentDateTime() ) >= 4000 )
				num++, dateTime = DateTimeService::currentDateTime();
			return ( block + 1 ) + 100 * num;
		}
	};

	class Sensor : public AbstractBRSensor
	{
	public:
		class Data : public SensorData
		{
			friend class Sensor;
		public:
			uint32_t v = 41;
		} data;

		const char* name() const final override
		{
			return "Sensor";
		}
		SensorData* readData() final override
		{
			data.lock();
			if( userData() == 1 && data.v == 41 )
				data.v = 141;
			++data.v;
			data.t = DateTimeService::currentDateTime();
			data.unlock();
			return &data;
		}
		SensorData* sensorData() final override
		{
			return &data;
		}
		uint32_t sensorDataSize() final override
		{
			return sizeof( Data ) - sizeof( SensorData );
		}
	};

	uint32_t numberOfSetBits( uint32_t i )
	{
		i = i - ( ( i >> 1 ) & 0x55555555 );
		i = ( i & 0x33333333 ) + ( ( i >> 2 ) & 0x33333333 );
		return ( ( ( i + ( i >> 4 ) ) & 0x0F0F0F0F ) * 0x01010101 ) >> 24;
	}

	int test()
	{
		DateTime dateTime = DateTimeService::currentDateTime();
		if( dateTime.time().hour() == 23 &&
			dateTime.time().min() >= 58 )
			return 0;

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
		File* file = new File;
		char* buffer = new char[1024];

		assert( fs->mount( "/" ) );
		crcStart( &CRCD1, nullptr );

		Dir( "/LogTest" ).removeRecursively();

		const char* path[] = { "2014/5/1", "2014/5/2", "2014/5/3", "2014/5/4", "2014/5/5",
							   "2015/6/6", "2015/6/7", "2015/6/8", "2015/6/9", "2015/6/10",
							   "2016/7/11" };
		for( int i = 0; i < 11; ++i )
		{
			Dir( "/LogTest" ).mkpath( path[i] );
			assert( file->open( ( std::string( "/LogTest/" ) + path[i] + "/gg.txt" ).c_str(), FileSystem::OpenAppend | FileSystem::Write ) );
			file->write( ( uint8_t* )0x20000000, 1024 * 100 ); // 100 kB
			file->close();
		}

		dateTime = DateTimeService::currentDateTime();

		SignalProviderA signalProvider;
		std::list < MarvieLog::SignalBlockDesc > list;
		list.push_back( MarvieLog::SignalBlockDesc{ 16, 7 } );
		list.push_back( MarvieLog::SignalBlockDesc{ 0, 6 } );
		list.push_back( MarvieLog::SignalBlockDesc{ 16, 0 } );

		MarvieLog* logSystem = new MarvieLog;
		logSystem->setSignalProvider( &signalProvider );
		logSystem->setMaxSize( 1 );
		logSystem->setOnlyNewDigitSignal( true );
		logSystem->setDigitSignalPeriod( 1000 );
		logSystem->setAnalogSignalPeriod( 2000 );
		logSystem->setSignalBlockDescList( list );
		logSystem->setRootPath( "/LogTest" );
		logSystem->startLogging();
		chThdSleepSeconds( 2 );

		Sensor sensor[2];
		sensor[0].setUserData( 0 );
		sensor[1].setUserData( 1 );
		sensor[0].readData();
		sensor[1].readData();
		logSystem->updateSensor( sensor );
		logSystem->updateSensor( sensor + 1 );
		logSystem->updateSensor( sensor );
		logSystem->updateSensor( sensor + 1 );
		chThdSleepSeconds( 2 );
		sensor[0].readData();
		sensor[1].readData();
		logSystem->updateSensor( sensor );
		logSystem->updateSensor( sensor + 1 );
		logSystem->updateSensor( sensor );
		logSystem->updateSensor( sensor + 1 );
		chThdSleepSeconds( 2 );
		sensor[0].readData();
		sensor[1].readData();
		logSystem->updateSensor( sensor );
		logSystem->updateSensor( sensor + 1 );
		logSystem->updateSensor( sensor );
		logSystem->updateSensor( sensor + 1 );
		chThdSleepMilliseconds( 500 );

		logSystem->stopLogging();
		logSystem->waitForStop();

		Date date = DateTimeService::currentDateTime().date();
		sprintf( buffer, "/LogTest/%d/%d/%d/DI.bin", ( int )date.year(), ( int )date.month(), ( int )date.day() );
		assert( file->open( buffer, FileSystem::Read | FileSystem::OpenExisting ) );

		uint8_t* data = ( uint8_t* )buffer;
		uint size = file->read( data, 1024 );
		DateTime dateTimeMark;
		int n = 0;
		while( size )
		{
			assert( n < 2 );
			assert( *( uint32_t * )data == 0x676f6c5f );
			if( n == 0 )
			{
				dateTimeMark = *( DateTime * )( data + 4 );
				assert( dateTime.msecsTo( dateTimeMark ) <= 1500 )
			}
			else
			{
				int64_t dt = dateTimeMark.msecsTo( *( DateTime * )( data + 4 ) );
				assert( abs( dt - 3000 ) < 150 );
				dateTimeMark = *( DateTime * )( data + 4 );
			}

			uint8_t flags = *( uint8_t * )( data + 4 + sizeof( DateTime ) );
			uint32_t blockCount = numberOfSetBits( flags );
			assert( flags == 5 && blockCount == 2 );

			Crc32HW::acquire();
			uint32_t crc = Crc32HW::crc32Byte( data, 4 + sizeof( DateTime ) + 1 + sizeof( uint32_t ) * blockCount, 0xFFFFFFFF );
			assert( crc == *( uint32_t * )( data + 4 + sizeof( DateTime ) + 1 + sizeof( uint32_t ) * blockCount ) );
			Crc32HW::release();

			uint32_t blockData[2] = { *( uint32_t * )( data + 4 + sizeof( DateTime ) + 1 ),
				*( uint32_t * )( data + 4 + sizeof( DateTime ) + 1 + 4 ) };
			if( n == 0 )
			{
				assert( blockData[0] == 1UL );
				assert( blockData[1] == 3UL );
			}
			else
			{
				assert( blockData[0] == 1UL );
				assert( blockData[1] == 3UL + 100 );
			}
			
			data += 4 + sizeof( DateTime ) + 1 + sizeof( uint32_t ) * blockCount + 4;
			size -= 4 + sizeof( DateTime ) + 1 + sizeof( uint32_t ) * blockCount + 4;
			++n;
		}
		file->close();

		sprintf( buffer, "/LogTest/%d/%d/%d/AI.bin", ( int )date.year(), ( int )date.month(), ( int )date.day() );
		assert( file->open( buffer, FileSystem::Read | FileSystem::OpenExisting ) );
		data = ( uint8_t* )buffer;
		size = file->read( data, 1024 );
		n = 0;
		while( size )
		{
			assert( *( uint32_t * )data == 0x676f6c5f );
			if( n == 0 )
			{
				dateTimeMark = *( DateTime * )( data + 4 );
				assert( dateTime.msecsTo( dateTimeMark ) <= 2500 )
			}
			else
			{
				int64_t dt = dateTimeMark.msecsTo( *( DateTime * )( data + 4 ) );
				assert( abs( dt - 2000 ) < 150 );
				dateTimeMark = *( DateTime * )( data + 4 );
			}

			uint16_t desc = *( uint8_t * )( data + 4 + sizeof( DateTime ) );
			uint16_t channelCount = 0;
			for( uint32_t i = 0; i < 8; ++i )
				channelCount += ( ( desc >> ( i * 2 ) ) & 0x03 ) * 8;
			assert( channelCount == 16 );

			Crc32HW::acquire();
			uint32_t crc = Crc32HW::crc32Byte( data, 4 + sizeof( DateTime ) + 2 + sizeof( uint32_t ) * channelCount, 0xFFFFFFFF );
			assert( crc == *( uint32_t * )( data + 4 + sizeof( DateTime ) + 2 + sizeof( uint32_t ) * channelCount ) );
			Crc32HW::release();

			for( int i = 0; i < 16; ++i )
			{
				float value = *( float * )( data + 4 + sizeof( DateTime ) + 2 + 4 * i );
				if( i == 7 || i == 15 || i == 14 )
					assert( value == -1 )
				else if( i < 8 )
					assert( value == i )
				else
					assert( value == i - 8 + 100 );
			}

			data += 4 + sizeof( DateTime ) + 2 + sizeof( float ) * channelCount + 4;
			size -= 4 + sizeof( DateTime ) + 2 + sizeof( float ) * channelCount + 4;
			++n;
		}
		assert( n == 3 );
		file->close();

		for( int iSensor = 1; iSensor <= 2; ++iSensor )
		{
			sprintf( buffer, "/LogTest/%d/%d/%d/%d.Sensor.bin", ( int )date.year(), ( int )date.month(), ( int )date.day(), iSensor );
			assert( file->open( buffer, FileSystem::Read | FileSystem::OpenExisting ) );
			data = ( uint8_t* )buffer;
			size = file->read( data, 1024 );
			n = 0;
			while( size )
			{
				assert( *( uint32_t * )data == 0x676f6c5f );
				if( n == 0 )
				{
					dateTimeMark = *( DateTime * )( data + 4 );
					assert( dateTime.msecsTo( dateTimeMark ) <= 2500 )
				}
				else
				{
					int64_t dt = dateTimeMark.msecsTo( *( DateTime * )( data + 4 ) );
					assert( abs( dt - 2000 ) < 150 );
					dateTimeMark = *( DateTime * )( data + 4 );
				}

				Crc32HW::acquire();
				uint32_t crc = Crc32HW::crc32Byte( data, 4 + sizeof( DateTime ) + sizeof( uint32_t ), 0xFFFFFFFF );
				assert( crc == *( uint32_t * )( data + 4 + sizeof( DateTime ) + sizeof( uint32_t ) ) );
				Crc32HW::release();

				uint32_t sensorData = *( uint32_t * )( data + 4 + sizeof( DateTime ) );
				assert( sensorData == 42UL + n + ( iSensor == 1 ? 0 : 100 ) );

				data += 4 + sizeof( DateTime ) + sizeof( uint32_t ) + 4;
				size -= 4 + sizeof( DateTime ) + sizeof( uint32_t ) + 4;
				++n;
			}
			assert( n == 3 );
			file->close();
		}

		assert( logSystem->size() == Dir( "/LogTest" ).contentSize() );

		size_t m0 = chCoreGetStatusX();
		systime_t t0 = chVTGetSystemTimeX();
		logSystem->startLogging();
		while( chVTTimeElapsedSinceX( t0 ) < TIME_S2I( 60 ) )
		{
			sensor[0].readData();
			sensor[1].readData();
			logSystem->updateSensor( sensor );
			logSystem->updateSensor( sensor + 1 );
			logSystem->updateSensor( sensor );
			logSystem->updateSensor( sensor + 1 );
			chThdYield();
		}
		logSystem->stopLogging();
		logSystem->waitForStop();

		size_t dm = chCoreGetStatusX() - m0;
		assert( dm < 256 );
		assert( logSystem->size() == Dir( "/LogTest" ).contentSize() );
		Dir( "/LogTest" ).removeRecursively();

		delete logSystem;
		delete buffer;
		delete file;

		crcStop( &CRCD1 );
		sdcDisconnect( &SDCD1 );
		sdcStop( &SDCD1 );
		delete fs;
		return 0;
	}
}