#include "Core/MemoryStatus.h"
#include "Core/ThreadsQueue.h"
#include "FirmwareTransferService.h"

namespace FirmwareTransferServiceTest
{
	ThreadsQueue tq;
	int test()
	{
		palSetPadMode( GPIOC, 8, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
		palSetPadMode( GPIOC, 9, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
		palSetPadMode( GPIOC, 10, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
		palSetPadMode( GPIOC, 11, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
		palSetPadMode( GPIOC, 12, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
		palSetPadMode( GPIOD, 2, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
		sdcStart( &SDCD1, nullptr );
		sdcConnect( &SDCD1 );

		FileSystem* fs = new FileSystem;
		assert( fs->mount( "/" ) );
		Dir( "Temp" ).removeRecursively();

		auto conf = EthernetThread::instance()->currentConfig();
		conf.addressMode = EthernetThread::AddressMode::Static;
		conf.ipAddress = IpAddress( 192, 168, 10, 10 );
		conf.netmask = 0xFFFFFF00;
		conf.gateway = IpAddress( 192, 168, 10, 1 );
		EthernetThread::instance()->setConfig( conf );

		//auto mem0 = MemoryStatus::freeSpace( MemoryStatus::Region::All );
		EthernetThread::instance()->startThread();

		class Callback : public FirmwareTransferService::Callback
		{
			virtual const char* firmwareVersion() override { return "0.8.2.1L"; }
			virtual const char* bootloaderVersion() override { return "1.1.0.0"; }
			virtual void firmwareDownloaded( const std::string& fileName ) override{
				//tq.dequeueNext( MSG_OK );
			};
			virtual void bootloaderDownloaded( const std::string& fileName ) override{
				//tq.dequeueNext( MSG_OK );
			};
			virtual void restartDevice() override
			{
				//tq.dequeueNext( MSG_OK );
			};
		} callback;
		FirmwareTransferService* service = new FirmwareTransferService;
		service->setCallback( &callback );
		service->setPassword( "admin" );
		service->startService();
		tq.enqueueSelf( TIME_INFINITE );
		service->stopService();
		service->waitForStopped();

		delete service;
		EthernetThread::instance()->stopThread();
		EthernetThread::instance()->waitForStop();
		//Thread::sleep( TIME_MS2I( 2000 ) );
		//assert( mem0 == MemoryStatus::freeSpace( MemoryStatus::Region::All ) );

		delete fs;
		sdcDisconnect( &SDCD1 );
		sdcStop( &SDCD1 );

		return 0;
	}
} // namespace FirmwareTransferServiceTest