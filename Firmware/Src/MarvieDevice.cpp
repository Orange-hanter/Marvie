#include "MarvieDevice.h"
#include "Core/CCMemoryAllocator.h"
#include "Core/CCMemoryHeap.h"
#include "Core/DateTimeService.h"
#include "Core/MutexLocker.h"
#include "FileSystem/FileInfoReader.h"
#include "Network/PingService.h"
#include "Network/TcpServer.h"
#include "Network/UdpSocket.h"
#include "Q.hpp"
#include "RemoteTerminal/CommandLineUtility.h"
#include "lwip/apps/sntp.h"
#include "lwip/dns.h"
#include "lwipthread.h"
#include "stm32f4xx_flash.h"

#include "Tests/UdpStressTestServer/UdpStressTestServer.h"

using namespace MarvieXmlConfigParsers;

MarvieDevice* MarvieDevice::_inst = nullptr;
char _bootloaderVersion[15 + 1];

MarvieDevice::MarvieDevice() : configXmlDataSendingSemaphore( false )
{
	crcStart( &CRCD1, nullptr );
	MarvieBackup* backup = MarvieBackup::instance();
	backup->init();
	incorrectShutdown = backup->failureDesc.pwrDown.power;
	backup->failureDesc.pwrDown.power = true;
	startDateTime = DateTimeService::currentDateTime();

	MarviePlatform::comPortAssignments( comPortAssignments );

	auto usartAF = []( SerialDriver* sd )
	{
		if( sd == &SD1 )
			return PAL_MODE_ALTERNATE( GPIO_AF_USART1 );
		if( sd == &SD2 )
			return PAL_MODE_ALTERNATE( GPIO_AF_USART2 );
		if( sd == &SD3 )
			return PAL_MODE_ALTERNATE( GPIO_AF_USART3 );
		if( sd == &SD4 )
			return PAL_MODE_ALTERNATE( GPIO_AF_UART4 );
		if( sd == &SD5 )
			return PAL_MODE_ALTERNATE( GPIO_AF_UART5 );
		if( sd == &SD6 )
			return PAL_MODE_ALTERNATE( GPIO_AF_USART6 );
		return PAL_MODE_ALTERNATE( 0 );
	};

	palSetPadMode( GPIOC, 0, PAL_MODE_INPUT_ANALOG );
	analogInputAddress[0].attach( MarviePlatform::analogInputAddressSelectorPorts[0] );
	analogInputAddress[1].attach( MarviePlatform::analogInputAddressSelectorPorts[1] );
	analogInputAddress[2].attach( MarviePlatform::analogInputAddressSelectorPorts[2] );
	analogInputAddress[3].attach( MarviePlatform::analogInputAddressSelectorPorts[3] );

	for( uint32_t i = 0; i < MarviePlatform::digitInputsCount; ++i )
		palSetPadMode( MarviePlatform::digitInputPorts[i].gpio, MarviePlatform::digitInputPorts[i].pinNum, PAL_MODE_INPUT_PULLDOWN );
	digitInputs = 0;

	adInputsReadThreadRef = nullptr;
	adcStart( &ADCD1, nullptr );

	for( uint i = 0; i < MarviePlatform::comUsartsCount; ++i )
	{
		comUsarts[i].usart = Usart::instance( MarviePlatform::comUsartIoLines[i].sd );
		palSetPadMode( MarviePlatform::comUsartIoLines[i].rxPort.gpio,
					   MarviePlatform::comUsartIoLines[i].rxPort.pinNum,
					   usartAF( MarviePlatform::comUsartIoLines[i].sd ) );
		palSetPadMode( MarviePlatform::comUsartIoLines[i].txPort.gpio,
					   MarviePlatform::comUsartIoLines[i].txPort.pinNum,
					   usartAF( MarviePlatform::comUsartIoLines[i].sd ) );
		comUsartInputBuffers[i] = ( uint8_t* )CCMemoryHeap::alloc( COM_USART_INPUT_BUFFER_SIZE );
		comUsarts[i].usart->setInputBuffer( comUsartInputBuffers[i], COM_USART_INPUT_BUFFER_SIZE );
		if( i == MarviePlatform::gsmModemComPort )
		{
			constexpr uint32_t size = COM_USART_OUTPUT_BUFFER_SIZE > GSM_MODEM_OUTPUT_BUFFER ? COM_USART_OUTPUT_BUFFER_SIZE : GSM_MODEM_OUTPUT_BUFFER;
			comUsartOutputBuffers[i] = ( uint8_t* )CCMemoryHeap::alloc( size );
			comUsarts[i].usart->setOutputBuffer( comUsartOutputBuffers[i], size );
		}
		else
		{
			comUsartOutputBuffers[i] = ( uint8_t* )CCMemoryHeap::alloc( COM_USART_OUTPUT_BUFFER_SIZE );
			comUsarts[i].usart->setOutputBuffer( comUsartOutputBuffers[i], COM_USART_OUTPUT_BUFFER_SIZE );
		}
		comUsarts[i].usart->open();
	}
	for( uint i = 0; i < MarviePlatform::comPortsCount; ++i )
		++( uint32_t& )comUsarts[MarviePlatform::comPortIoLines[i].comUsartIndex].sharedRs485Control;
	for( uint i = 0; i < MarviePlatform::comUsartsCount; ++i )
	{
		if( ( uint32_t )comUsarts[i].sharedRs485Control > 1 )
			comUsarts[i].sharedRs485Control = new SharedRs485Control( comUsarts[i].usart );
		else
			comUsarts[i].sharedRs485Control = nullptr;
	}
	for( uint i = 0; i < MarviePlatform::comPortsCount; ++i )
	{
		auto& comUsart = comUsarts[MarviePlatform::comPortIoLines[i].comUsartIndex];
		if( MarviePlatform::comPortTypes[i] == ComPortType::Rs232 )
			comPorts[i] = comUsart.usart;
		else // ComPortType::Rs485
		{
			auto& control = comUsart.sharedRs485Control;
			if( control )
				comPorts[i] = control->create( MarviePlatform::comPortIoLines[i].rePort, MarviePlatform::comPortIoLines[i].dePort );
			else
				comPorts[i] = new Rs485( comUsart.usart, MarviePlatform::comPortIoLines[i].rePort, MarviePlatform::comPortIoLines[i].dePort );
		}
	}

	// power down pin config
	palSetPadMode( GPIOC, 13, PAL_MODE_INPUT );
	palSetPadCallbackI( GPIOC, 13, powerDownExtiCallback, ( void* )13 );

	// SimGsm PWR KEY // TEMP!
	palSetPadMode( GPIOD, 1, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST );
	if( MarviePlatform::coreVersion[strlen( MarviePlatform::coreVersion ) - 1] == 'L' )
		palClearPad( GPIOD, 1 );
	else
		palSetPad( GPIOD, 1 );

	// PA8 - CD
	palSetPadMode( GPIOC, 8, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );    // D0
	//palSetPadMode( GPIOC, 9, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );  // D1
	//palSetPadMode( GPIOC, 10, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST ); // D2
	palSetPadMode( GPIOC, 11, PAL_MODE_INPUT | PAL_STM32_OSPEED_HIGHEST );               // D3/CD
	palSetPadMode( GPIOC, 12, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );   // CLK
	palSetPadMode( GPIOD, 2, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );    // CMD
	static SDCConfig sdcConfig;
	sdcConfig.scratchpad = nullptr;
	sdcConfig.bus_width = SDC_MODE_1BIT;
	sdcStart( &SDCD1, &sdcConfig/*nullptr*/ );
	sdCardStatus = SdCardStatus::NotInserted;
	sdCardInserted = false;
	sdCardStatusEventPending = false;
	f_mount( &fatFs, "/", 0 );
	fsError = FR_NOT_READY;

	deviceState = DeviceState::IncorrectConfiguration;
	configError = ConfigError::NoError;
	sensorsConfigError = SensorsConfigError::NoError;
	errorSensorTypeName = nullptr;
	errorSensorId = 0;
	srSensorPeriodCounter = SrSensorPeriodCounterLoad;
	monitoringLogSize = 0;

	configNum = 0;
	for( uint i = 0; i < MarviePlatform::comPortsCount; ++i )
	{
		comPortCurrentAssignments[i] = MarvieXmlConfigParsers::ComPortAssignment::VPort;
		comPortServiceRoutines[i] = nullptr;
		comPortBlockFlags[i] = false;
		vPortToComPortMap[i] = -1;
	}
	vPorts = nullptr;
	vPortBindings = nullptr;
	vPortsCount = 0;
	sensors = nullptr;
	sensorsCount = 0;
	brSensorReaders = nullptr;
	brSensorReaderListeners = nullptr;
	marvieLog = nullptr;
	sensorLogEnabled = false;
	gsmModem = nullptr;
	rawModbusServers = nullptr;
	rawModbusServersCount = 0;
	tcpModbusRtuServer = nullptr;
	tcpModbusAsciiServer = nullptr;
	tcpModbusIpServer = nullptr;
	modbusRegisters = nullptr;
	modbusRegistersCount = 0;
	sharedComPortIndex = -1;

	syncConfigNum = ( uint32_t )-1;
	datFiles[0] = datFiles[1] = datFiles[2] = nullptr;
	networkSharedComPortThread = nullptr;
	sharedComPortNetworkClientsCount = -1;

	memoryLoad.totalGRam = 128 * 1024;
	memoryLoad.totalCcRam = 64 * 1024;
	memoryLoad.freeGRam = 0;
	memoryLoad.gRamHeapFragments = 0;
	memoryLoad.gRamHeapSize = 0;
	memoryLoad.gRamHeapLargestFragmentSize = 0;
	memoryLoad.freeCcRam = 0;
	memoryLoad.ccRamHeapFragments = 0;
	memoryLoad.ccRamHeapSize = 0;
	memoryLoad.ccRamHeapLargestFragmentSize = 0;
	memoryLoad.sdCardCapacity = 0;
	memoryLoad.sdCardFreeSpace = 0;

	ip_addr_t dnsAddr;
	IP4_ADDR( &dnsAddr, 8, 8, 8, 8 );
	dns_setserver( 0, &dnsAddr );

	auto ethConf = EthernetThread::instance()->currentConfig();
	ethConf.addressMode = backup->settings.flags.ethernetDhcp ? EthernetThread::AddressMode::Dhcp : EthernetThread::AddressMode::Static;
	ethConf.ipAddress = backup->settings.eth.ip;
	ethConf.netmask = backup->settings.eth.netmask;
	ethConf.gateway = backup->settings.eth.gateway;
	EthernetThread::instance()->setConfig( ethConf );
	EthernetThread::instance()->startThread( EthernetThreadPrio );

	mLinkServer = new MLinkServer;
	mLinkServer->setAuthenticationCallback( this );
	mLinkServer->setDataChannelCallback( this );
	terminalServer.setOutputCallback( terminalOutput, this );
	terminalServer.registerFunction( "ls", CommandLineUtility::ls );
	terminalServer.registerFunction( "mkdir", CommandLineUtility::mkdir );
	terminalServer.registerFunction( "rm", CommandLineUtility::rm );
	terminalServer.registerFunction( "mv", CommandLineUtility::mv );
	terminalServer.registerFunction( "cat", CommandLineUtility::cat );
	terminalServer.registerFunction( "tail", CommandLineUtility::tail );
	terminalServer.registerFunction( "ping", CommandLineUtility::ping );
	terminalServer.registerFunction( "rtc", rtc );
	terminalServer.registerFunction( "lgbt", lgbt );
	terminalServer.registerFunction( "Q", qAsciiArtFunction );
	terminalOutputTimer.setParameter( this );
	terminalServer.startServer();

	PingService::instance()->startService();
	testIpAddr = IpAddress( 8, 8, 8, 8 );

	mainThread = nullptr;
	miskTasksThread = nullptr;
	adInputsReadThread = nullptr;
	mLinkServerHandlerThread = nullptr;
	terminalOutputThread = nullptr;
	uiThread = nullptr;
	networkServiceThread = nullptr;
}

MarvieDevice* MarvieDevice::instance()
{
	if( _inst )
		return _inst;
	_inst = new MarvieDevice;
	return _inst;
}

void MarvieDevice::exec()
{
	mainThread               = Thread::createAndStart( ThreadProperties( 2560, NORMALPRIO ), [this]() { mainThreadMain(); } );
	miskTasksThread          = Thread::createAndStart( ThreadProperties( 2048, NORMALPRIO ), [this]() { miskTasksThreadMain(); } );
	adInputsReadThread       = Thread::createAndStart( ThreadProperties( 512,  NORMALPRIO + 2 ), [this]() { adInputsReadThreadMain(); } );
	mLinkServerHandlerThread = Thread::createAndStart( ThreadProperties( 2048, NORMALPRIO ), [this]() { mLinkServerHandlerThreadMain(); } );
	terminalOutputThread     = Thread::createAndStart( ThreadProperties( 1536 ), [this]() { terminalOutputThreadMain(); } );
	uiThread                 = Thread::createAndStart( ThreadProperties( 1024, NORMALPRIO ), [this]() { uiThreadMain(); } );
	networkServiceThread     = Thread::createAndStart( ThreadProperties( 1536, NORMALPRIO ), [this]() { networkServiceThreadMain(); } );

	Concurrent::run( ThreadProperties( 2048, NORMALPRIO, "UdpStressTestServerThread" ), []()
	{
		UdpStressTestServer* server = new UdpStressTestServer( 1112 );
		server->exec();
	} );
	/*Concurrent::run( ThreadProperties( 2048, NORMALPRIO, "UdpStressTestServerThread2" ), []()
	{
		UdpStressTestServer* server = new UdpStressTestServer( 1114 );
		server->exec();
	} );*/

	Concurrent::run( ThreadProperties( 2048, NORMALPRIO, "" ), [this]()
	{
		StaticTimer<> timer( []() {
			__NVIC_SystemReset();
		} );
		timer.setInterval( TIME_MS2I( 16000 ) );

		UdpSocket* socket = new UdpSocket;
		uint8_t buf[32];
		socket->bind( 42666 );
		while( true )
		{
			if( !socket->waitDatagram( TIME_MS2I( 1000 ) ) )
				continue;
			IpAddress addr;
			uint16_t port;
			socket->readDatagram( buf, sizeof( buf ), &addr, &port );
			if( strcmp( ( const char* )buf, "@ZX#42%0?gmt@Re@zY_" ) == 0 )
			{
				for( int i = 0; i < 5; ++i )
				{
					socket->writeDatagram( ( const uint8_t* )"ok", 2, addr, port );
					chThdSleepMilliseconds( 200 );
				}
				mainThread->signalEvents( MainThreadEvent::RestartRequestEvent );
				timer.start();
			}
			else if( strcmp( ( const char* )buf, "@ZX#42%0?gmt@DeMv@zY_" ) == 0 )
			{
				for( int i = 0; i < 5; ++i )
				{
					socket->writeDatagram( ( const uint8_t* )"ok", 2, addr, port );
					chThdSleepMilliseconds( 200 );
				}
				FLASH_Unlock();
				FLASH_EraseAllBank1Sectors( VoltageRange_3 );
			}
		}
	} );

	/*TcpServer* server = new TcpServer;
	server->listen( 42420 );
	while( server->isListening() )
	{
		if( server->waitForNewConnection( TIME_MS2I( 100 ) ) )
		{
			TcpSocket* socket = server->nextPendingConnection();
			Concurrent::run( [socket]()
			{
				uint8_t* data = new uint8_t[2048];
				while( true )
				{
					if( socket->waitForReadAvailable( 1, TIME_MS2I( 100 ) ) )
					{
						while( socket->readAvailable() )
						{
							uint32_t len = socket->read( data, 2048, TIME_IMMEDIATE );
							if( socket->write( data, len ) == 0 )
								goto End;
						}
					}
					if( !socket->isOpen() )
						break;
				}
End:
				delete data;
				delete socket;
			}, 2048, NORMALPRIO );
		}
	}*/

	thread_reference_t ref = nullptr;
	chSysLock();
	chThdSuspendS( &ref );
}

void MarvieDevice::mainThreadMain()
{
	palEnablePadEvent( GPIOC, 13, PAL_EVENT_MODE_FALLING_EDGE );

	configMutex.lock();
	MarvieBackup* backup = MarvieBackup::instance();
	if( backup->settings.flags.gsmEnabled )
	{
		createGsmModemObjectM();
		gsmModem->setPinCode( backup->settings.gsm.pinCode );
		gsmModem->setApn( backup->settings.gsm.apn );
		gsmModem->startModem();

		comPortCurrentAssignments[MarviePlatform::gsmModemComPort] = MarvieXmlConfigParsers::ComPortAssignment::GsmModem;
		comPortServiceRoutines[MarviePlatform::gsmModemComPort] = gsmModem;
	}
	configMutex.unlock();

	firmwareTransferService.setCallback( this );
	firmwareTransferService.setPassword( backup->settings.passwords.adminPassword );
	firmwareTransferService.startService();

	while( true )
	{
		eventmask_t em = chEvtWaitAny( ALL_EVENTS );
		if( em & MainThreadEvent::PowerDownDetected )
		{
			ejectSdCard();
			backup->failureDesc.pwrDown.power = false;
			chSysLock();
			while( !palReadPad( GPIOC, 13 ) )
				;
			__NVIC_SystemReset();
		}
		if( em & MainThreadEvent::SdCardStatusChanged )
		{
			if( sdCardInserted )
			{
				sdCardStatus = SdCardStatus::Initialization;
				if( sdcConnect( &SDCD1 ) != HAL_SUCCESS )
					sdCardStatus = SdCardStatus::InitFailed;
				else
				{
					fsError = f_mount( &fatFs, "/", 1 );
					if( fsError == FR_OK && fatFs.fs_type == FM_EXFAT )
					{
						DWORD nclst;
						FATFS* fs;
						f_getfree( "0:", &nclst, &fs );
						fsError = clearDir( "/Temp" );
						if( fsError == FR_NO_PATH )
							fsError = f_mkdir( "/Temp" );
						if( fsError == FR_OK )
						{
							sdCardStatus = SdCardStatus::Working;
							Dir( "/Log" ).mkdir();
							fileLog.open( "/Log/log.txt" );
							if( configNum == 0 )
							{
								sprintf( ( char* )buffer, "System started after %s shutdown", incorrectShutdown ? "incorrect" : "correct" );
								fileLog.addRecorg( ( char* )buffer );
							}
							logFailure();
							auto file = findBootloaderFile();
							if( file )
								updateBootloader( file.get() );
							monitoringLogSize = Dir( "/Log/Monitoring" ).contentSize();
							if( marvieLog )
								marvieLog->startLogging( LOWPRIO );
							reconfig();
						}
						else
							sdCardStatus = SdCardStatus::InitFailed;
					}
					else
						sdCardStatus = SdCardStatus::BadFileSystem;
				}
			}
			else
				ejectSdCard();
			sdCardStatusEventPending = false;
		}
		if( em & MainThreadEvent::NewXmlConfigDatFile )
		{
			datFilesMutex.lock();
			auto& file = datFiles[MarviePackets::ComplexChannel::XmlConfigChannel];
			if( file )
			{
				char path[] = "/Temp/0.dat";
				path[6] = '0' + MarviePackets::ComplexChannel::XmlConfigChannel;

				configXmlFileMutex.lock();
				f_unlink( "/config.xml" );
				if( f_close( file ) == FR_OK && f_rename( path, "/config.xml" ) == FR_OK )
				{
					fileLog.addRecorg( "New configuration is downloaded" );
					configXmlFileMutex.unlock();
					reconfig();
				}
				else
					configXmlFileMutex.lock();
				delete file;
				file = nullptr;
			}
			datFilesMutex.unlock();
		}
		if( em & MainThreadEvent::StartSensorReaders )
		{
			for( uint i = 0; i < vPortsCount; ++i )
			{
				if( i < MarviePlatform::comPortsCount && vPortToComPortMap[i] != -1 && comPortBlockFlags[vPortToComPortMap[i]] )
					continue;
				if( brSensorReaders[i] )
					brSensorReaders[i]->startReading( NORMALPRIO );
			}
		}
		if( em & MainThreadEvent::StopSensorReaders )
		{
			for( uint i = 0; i < vPortsCount; ++i )
			{
				if( i < MarviePlatform::comPortsCount && vPortToComPortMap[i] != -1 && comPortBlockFlags[vPortToComPortMap[i]] )
					continue;
				if( brSensorReaders[i] )
					brSensorReaders[i]->stopReading();
			}
		}
		if( em & MainThreadEvent::BrSensorReaderEvent )
		{
			volatile bool updated = false;
			for( uint i = 0; i < vPortsCount; ++i )
			{
				eventflags_t flags = brSensorReaderListeners[i].getAndClearFlags();
				if( flags & ( eventflags_t )BRSensorReader::EventFlag::SensorDataUpdated )
				{
					updated = true;
					if( vPortBindings[i] == VPortBinding::Rs485 )
					{
						MultipleBRSensorsReader* reader = static_cast< MultipleBRSensorsReader* >( brSensorReaders[i] );
						AbstractBRSensor* sensor = reader->nextUpdatedSensor();
						while( sensor )
						{
							if( !sensor->sensorData()->isValid() )
								sensors[sensor->userData()].dateTime = DateTimeService::currentDateTime();
							sensors[sensor->userData()].updatedForMLink = true;
							copySensorDataToModbusRegisters( sensor );
							if( marvieLog && sensorLogEnabled )
								marvieLog->updateSensor( sensor, &sensors[sensor->userData()].sensorName );
							sensor = reader->nextUpdatedSensor();
						}
					}
					else if( vPortBindings[i] == VPortBinding::Rs232 || vPortBindings[i] == VPortBinding::NetworkSocket )
					{
						AbstractBRSensor* sensor = static_cast< SingleBRSensorReader* >( brSensorReaders[i] )->nextSensor();
						if( !sensor->sensorData()->isValid() )
							sensors[sensor->userData()].dateTime = DateTimeService::currentDateTime();
						sensors[sensor->userData()].updatedForMLink = true;
						copySensorDataToModbusRegisters( sensor );
						if( marvieLog && sensorLogEnabled )
							marvieLog->updateSensor( sensor, &sensors[sensor->userData()].sensorName );
					}
					else
					{
						// TODO: add code here?
					}
				}
			}
			if( updated )
				mLinkServerHandlerThread->signalEvents( MLinkThreadEvent::SensorUpdateEvent );
		}
		if( em & MainThreadEvent::SrSensorsTimerEvent )
		{
			volatile bool mLinkSensorUpdated = false;
			srSensorsUpdateTimer.setParameter( MainThreadEvent::SrSensorsTimerEvent );
			srSensorsUpdateTimer.start( TIME_MS2I( SrSensorInterval ) );
			--srSensorPeriodCounter;
			for( uint i = 0; i < sensorsCount; ++i )
			{
				if( sensors[i].sensor->type() == AbstractSensor::Type::SR )
				{
					if( sensors[i].dateTime != sensors[i].sensor->sensorData()->time() )
					{
						sensors[i].dateTime = sensors[i].sensor->sensorData()->time();
						sensors[i].readyForMLink = true;
						sensors[i].readyForLog = true;
					}

					if( srSensorPeriodCounter == 0 && sensors[i].readyForMLink )
					{
						mLinkSensorUpdated = true;
						sensors[i].updatedForMLink = true;
						sensors[i].readyForMLink = false;
						copySensorDataToModbusRegisters( sensors[i].sensor );
					}

					if( marvieLog && sensorLogEnabled )
					{
						if( sensors[i].logTimeLeft == 0 )
						{
							sensors[i].logTimeLeft = sensors[i].logPeriod;
							if( sensors[i].readyForLog )
							{
								sensors[i].readyForLog = false;
								marvieLog->updateSensor( sensors[i].sensor, &sensors[i].sensorName );
							}
						}
						else
							sensors[i].logTimeLeft -= SrSensorInterval;
					}
				}
			}
			if( srSensorPeriodCounter == 0 )
				srSensorPeriodCounter = SrSensorPeriodCounterLoad;
			if( mLinkSensorUpdated )
				mLinkServerHandlerThread->signalEvents( MLinkThreadEvent::SensorUpdateEvent );
		}
		if( em & MainThreadEvent::GsmModemMainEvent )
		{
			eventflags_t flags = gsmModemMainThreadListener.getAndClearFlags();
			if( gsmModem )
			{
				if( flags & ( eventflags_t )ModemEvent::StateChanged || flags & ( eventflags_t )ModemEvent::NetworkAddressChanged )
				{
					if( gsmModem->state() == ModemState::Initializing )
						fileLog.addRecorg( "Start gsm modem initializing" );
					else if( gsmModem->state() == ModemState::Working )
					{
						char* str = ( char* )buffer;
						IpAddress ip = gsmModem->networkAddress();
						sprintf( str, "Gsm modem online [%d.%d.%d.%d]", ip.addr[3], ip.addr[2], ip.addr[1], ip.addr[0] );
						fileLog.addRecorg( str );
					}
				}
			}
		}
		if( em & MainThreadEvent::RestartRequestEvent )
		{
			auto file = findBootloaderFile();
			if( file )
				updateBootloader( file.get() );

			firmwareTransferService.stopService();
			firmwareTransferService.waitForStopped();
			mLinkServer->stopListening();
			mLinkServer->waitForStateChanged();
			Thread::sleep( TIME_MS2I( 1000 ) );
			ejectSdCard();
			backup->failureDesc.pwrDown.power = false;
			__NVIC_SystemReset();
		}
		if( em & MainThreadEvent::EjectSdCardRequest )
			ejectSdCard();
		if( em & MainThreadEvent::NewFirmwareDatFile )
		{
			datFilesMutex.lock();
			auto& file = datFiles[MarviePackets::ComplexChannel::FirmwareChannel];
			if( file )
			{
				char path[] = "/Temp/0.dat";
				path[6] = '0' + MarviePackets::ComplexChannel::FirmwareChannel;

				f_unlink( "/firmware.bin" );
				f_close( file );
				f_rename( path, "/firmware.bin" );
				fileLog.addRecorg( "New firmware is downloaded" );

				delete file;
				file = nullptr;
			}
			datFilesMutex.unlock();
		}
		if( em & MainThreadEvent::NewBootloaderDatFile )
		{
			datFilesMutex.lock();
			auto& file = datFiles[MarviePackets::ComplexChannel::BootloaderChannel];
			if( file )
			{
				char path[] = "/Temp/0.dat";
				path[6] = '0' + MarviePackets::ComplexChannel::BootloaderChannel;

				f_unlink( "/bootloader.bin" );
				f_close( file );
				f_rename( path, "/bootloader.bin" );
				fileLog.addRecorg( "New bootloader is downloaded" );

				delete file;
				file = nullptr;
			}
			datFilesMutex.unlock();
		}
		if( em & MainThreadEvent::RestartGsmModemRequestEvent )
		{
			if( gsmModem && !comPortBlockFlags[MarviePlatform::gsmModemComPort] )
			{
				gsmModem->stopModem();
				gsmModem->waitForStateChange();
				gsmModem->startModem();
			}
		}
		if( em & MainThreadEvent::CleanMonitoringLogRequestEvent )
		{
			if( marvieLog )
				marvieLog->clean();
			else
			{
				Dir( "/Log/Monitoring" ).removeRecursively();
				monitoringLogSize = 0;
			}
		}
		if( em & MainThreadEvent::CleanSystemLogRequestEvent )
			fileLog.clean();
		if( em & MainThreadEvent::FormatSdCardRequest )
			formatSdCard();
		if( em & MainThreadEvent::StartComPortSharingEvent )
		{
			auto usartIndex = MarviePlatform::comPortIoLines[sharedComPortIndex].comUsartIndex;
			for( uint i = 0; i < MarviePlatform::comPortsCount; ++i )
			{
				if( MarviePlatform::comPortIoLines[i].comUsartIndex == usartIndex )
				{
					if( comPortServiceRoutines[i] )
					{
						switch( comPortCurrentAssignments[i] )
						{
						case MarvieXmlConfigParsers::ComPortAssignment::VPort:
							reinterpret_cast< BRSensorReader* >( comPortServiceRoutines[i] )->stopReading();
							reinterpret_cast< BRSensorReader* >( comPortServiceRoutines[i] )->waitForStateChange();
							break;
						case MarvieXmlConfigParsers::ComPortAssignment::GsmModem:
							reinterpret_cast< AbstractPppModem* >( comPortServiceRoutines[i] )->stopModem();
							reinterpret_cast< AbstractPppModem* >( comPortServiceRoutines[i] )->waitForStateChange();
							break;
						case MarvieXmlConfigParsers::ComPortAssignment::ModbusRtuSlave:
						case MarvieXmlConfigParsers::ComPortAssignment::ModbusAsciiSlave:
							reinterpret_cast< RawModbusServer* >( comPortServiceRoutines[i] )->stopServer();
							reinterpret_cast< RawModbusServer* >( comPortServiceRoutines[i] )->waitForStateChange();
							break;
						case MarvieXmlConfigParsers::ComPortAssignment::Multiplexer:
							break;
						default:
							break;
						}
					}
					comPortBlockFlags[i] = true;
				}
			}
			sharingComPortThreadQueue.dequeueNext( MSG_OK );
		}
		if( em & MainThreadEvent::StopComPortSharingEvent )
		{
			auto usartIndex = MarviePlatform::comPortIoLines[sharedComPortIndex].comUsartIndex;
			for( uint i = 0; i < MarviePlatform::comPortsCount; ++i )
			{
				if( MarviePlatform::comPortIoLines[i].comUsartIndex == usartIndex )
				{
					if( comPortServiceRoutines[i] )
					{
						switch( comPortCurrentAssignments[i] )
						{
						case MarvieXmlConfigParsers::ComPortAssignment::VPort:
							reinterpret_cast< BRSensorReader* >( comPortServiceRoutines[i] )->startReading( NORMALPRIO );
							break;
						case MarvieXmlConfigParsers::ComPortAssignment::GsmModem:
							configGsmModemUsart();
							reinterpret_cast< AbstractPppModem* >( comPortServiceRoutines[i] )->startModem();
							break;
						case MarvieXmlConfigParsers::ComPortAssignment::ModbusRtuSlave:
						case MarvieXmlConfigParsers::ComPortAssignment::ModbusAsciiSlave:
							reinterpret_cast< RawModbusServer* >( comPortServiceRoutines[i] )->startServer();
							break;
						case MarvieXmlConfigParsers::ComPortAssignment::Multiplexer:
							break;
						default:
							break;
						}
					}
					comPortBlockFlags[i] = false;
				}
			}
			sharingComPortThreadQueue.dequeueNext( MSG_OK );
		}
	}
}

void MarvieDevice::miskTasksThreadMain()
{
	BasicTimer< void(*)( eventmask_t ), &MarvieDevice::miskTaskThreadTimersCallback > sdTestTimer( MiskTaskThreadEvent::SdCardTestEvent );
	BasicTimer< void(*)( eventmask_t ), &MarvieDevice::miskTaskThreadTimersCallback > memoryTestTimer( MiskTaskThreadEvent::MemoryTestEvent );
	sdTestTimer.start( TIME_MS2I( SdTestInterval ) );
	memoryTestTimer.start( TIME_MS2I( MemoryTestInterval ) );
	uint32_t counter = 0;

	while( true )
	{
		eventmask_t em = chEvtWaitAny( ALL_EVENTS );
		if( em & MiskTaskThreadEvent::SdCardTestEvent )
		{
			auto status = blkIsInserted( &SDCD1 );
			if( sdCardInserted != status )
			{
				if( !sdCardInserted )
				{
					++counter;
					if( counter >= 30 && !sdCardStatusEventPending )
					{
						sdCardInserted = status;
						sdCardStatusEventPending = true;
						mainThread->signalEvents( MainThreadEvent::SdCardStatusChanged );
					}
				}
				else
				{
					counter = 0;
					sdCardInserted = status;
					sdCardStatusEventPending = true;
					mainThread->signalEvents( MainThreadEvent::SdCardStatusChanged );
				}
			}
			sdTestTimer.start( TIME_MS2I( SdTestInterval ) );
		}
		if( em & MiskTaskThreadEvent::MemoryTestEvent )
		{
			memoryLoad.freeGRam = chCoreGetStatusX();
			memoryLoad.freeCcRam = CCMemoryAllocator::status();
			memoryLoad.gRamHeapFragments = chHeapStatus( nullptr, ( size_t* )&memoryLoad.gRamHeapSize, ( size_t* )&memoryLoad.gRamHeapLargestFragmentSize );
			memoryLoad.ccRamHeapFragments = CCMemoryHeap::status( ( size_t* )&memoryLoad.ccRamHeapSize, ( size_t* )&memoryLoad.ccRamHeapLargestFragmentSize );
			if( sdCardStatus == SdCardStatus::Working )
			{
				memoryLoad.sdCardCapacity = ( unsigned long long )( fatFs.n_fatent - 2 ) * fatFs.csize * 512;
				memoryLoad.sdCardFreeSpace = ( unsigned long long )fatFs.free_clst * fatFs.csize * 512;
				chSysLock();
				if( marvieLog )
					memoryLoad.logSize = marvieLog->size();
				else
					memoryLoad.logSize = monitoringLogSize;
				chSysUnlock();
			}
			else
			{
				memoryLoad.sdCardCapacity = 0;
				memoryLoad.sdCardFreeSpace = 0;
				memoryLoad.logSize = 0;
			}

			mLinkServerHandlerThread->signalEvents( MLinkThreadEvent::MemoryLoadEvent );
			memoryTestTimer.start( TIME_MS2I( MemoryTestInterval ) );
		}
	}
}

void MarvieDevice::adInputsReadThreadMain()
{
	static const ADCConversionGroup adcConvGroup = {
		FALSE,                               // circular buffer mode
		1,                                   // number of the analog channels
		adcCallback,                         // callback function
		adcErrorCallback,                    // error callback
		0,                                   // CR1
		ADC_CR2_SWSTART,                     // CR2
		ADC_SMPR1_SMP_AN10( ADC_SAMPLE_28 ), // sample times for channel 10-18
		0,                                   // sample times for channel 0-9
		0,                                   // ADC SQR1 Conversion group sequence 13-16 + sequence length.
		0,                                   // ADC SQR2 Conversion group sequence 7-12
		ADC_SQR3_SQ1_N( ADC_CHANNEL_IN10 )   // ADC SQR3 Conversion group sequence 1-6
	};

	while( true )
	{
		systime_t t0 = chVTGetSystemTimeX();
		for( uint32_t i = 0; i < MarviePlatform::analogInputsCount; ++i )
		{
			analogInputAddress[0].setState( i & ( 1 << 0 ) );
			analogInputAddress[1].setState( i & ( 1 << 1 ) );
			analogInputAddress[2].setState( i & ( 1 << 2 ) );
			analogInputAddress[3].setState( i & ( 1 << 3 ) );
			chThdSleepMicroseconds( 1 );

			chSysLock();
			adcStartConversionI( &ADCD1, &adcConvGroup, adcSamples, 8 );
			chThdSuspendS( &adInputsReadThreadRef );
			chSysUnlock();

			uint32_t avg = 0;
			for( int iSample = 0; iSample < 8; ++iSample )
				avg += adcSamples[iSample];
			//TODO
			analogInput[i] = float( ( avg * 3300 ) / ( 4096 * 8 ) ) /*/ 1000*/;
		}

		chSysLock();
		digitInputs = 0;
		for( uint32_t i = 0; i < MarviePlatform::digitInputsCount; ++i )
			digitInputs |= palReadPad( MarviePlatform::digitInputPorts[i].gpio, MarviePlatform::digitInputPorts[i].pinNum ) << i;
		chSysUnlock();

		configMutex.lock();
		for( uint i = 0; i < sensorsCount; ++i )
		{
			if( sensors[i].sensor->type() == AbstractSensor::Type::SR )
				sensors[i].sensor->readData();
		}
		configMutex.unlock();

		sysinterval_t dt = chVTTimeElapsedSinceX( t0 );
		if( TIME_MS2I( MarviePlatform::srSensorUpdatePeriodMs ) > dt )
			chThdSleepMilliseconds( TIME_MS2I( MarviePlatform::srSensorUpdatePeriodMs ) - dt );
	}
}

void MarvieDevice::mLinkServerHandlerThreadMain()
{
	EventListener mLinkListener, ethThreadListener, cpuUsageMonitorListener;
	mLinkServer->eventSource().registerMask( &mLinkListener, MLinkThreadEvent::MLinkEvent );
	EthernetThread::instance()->eventSource().registerMask( &ethThreadListener, MLinkThreadEvent::EthernetEvent );
	CpuUsageMonitor::instance()->eventSource().registerMask( &cpuUsageMonitorListener, MLinkThreadEvent::CpuUsageMonitorEvent );

	BasicTimer< void(*)( eventmask_t ), &MarvieDevice::mLinkServerHandlerThreadTimersCallback > timer( MLinkThreadEvent::StatusUpdateEvent );

	syncConfigNum = ( uint32_t )-1;
	auto sensorDataChannel = mLinkServer->createDataChannel();

	mLinkServer->startListening( NORMALPRIO );
	while( true )
	{
		eventmask_t em = chEvtWaitAny( ALL_EVENTS );
		if( em & MLinkThreadEvent::MLinkEvent )
		{
			eventflags_t flags = mLinkListener.getAndClearFlags();
			if( flags & ( eventflags_t )MLinkServer::Event::StateChanged )
			{
				if( mLinkServer->state() == MLinkServer::State::Connected )
				{
					removeOpenDatFiles();
					mLinkServer->confirmSession();
					mLinkSync( true );
					timer.start( TIME_MS2I( StatusInterval ) );
				}
				else
					timer.stop(), terminalServer.reset();
			}
			if( flags & ( eventflags_t )MLinkServer::Event::NewPacketAvailable )
			{
				while( true )
				{
					uint8_t type;
					uint32_t size = mLinkServer->readPacket( &type, mLinkBuffer, sizeof( mLinkBuffer ) );
					if( type == 255 )
						break;
					mLinkProcessNewPacket( type, mLinkBuffer, size );
				}
			}
		}
		if( mLinkServer->state() != MLinkServer::State::Connected )
			continue;
		if( mLinkServer->accountIndex() != 0 )
		{
			if( em & MLinkThreadEvent::ConfigResetEvent )
			{
				mLinkServer->sendPacket( MarviePackets::Type::ConfigResetType, nullptr, 0 );
				syncConfigNum = ( uint32_t )-1;
			}
			if( em & MLinkThreadEvent::ConfigChangedEvent )
				mLinkSync( false );
			if( em & MLinkThreadEvent::SensorUpdateEvent )
			{
				configMutex.lock();
				if( syncConfigNum == configNum )
				{
					for( uint i = 0; i < sensorsCount; ++i )
					{
						if( !sensors[i].updatedForMLink )
							continue;
						sensors[i].updatedForMLink = false;
						sendSensorDataM( i, sensorDataChannel );
					}
				}
				configMutex.unlock();
			}
		}
		if( em & MLinkThreadEvent::StatusUpdateEvent )
		{
			timer.start( TIME_MS2I( StatusInterval ) );
			sendDeviceStatus();
			if( mLinkServer->accountIndex() != 0 )
			{
				sendAnalogInputsData();
				sendDigitInputsData();
			}

			configMutex.lock();
			sendServiceStatisticsM();
			if( mLinkServer->accountIndex() != 0 && syncConfigNum == configNum )
			{
				for( uint i = 0; i < vPortsCount; ++i )
					sendVPortStatusM( i );
			}
			configMutex.unlock();
		}
		if( em & MLinkThreadEvent::CpuUsageMonitorEvent )
		{
			MarviePackets::CpuLoad load;
			load.load = CpuUsageMonitor::instance()->usage();
			mLinkServer->sendPacket( MarviePackets::Type::CpuLoadType, ( uint8_t* )&load, sizeof( load ) );
		}
		if( em & MLinkThreadEvent::MemoryLoadEvent )
		{
			chSysLock();
			MarviePackets::MemoryLoad load = memoryLoad;
			chSysUnlock();
			mLinkServer->sendPacket( MarviePackets::Type::MemoryLoadType, ( uint8_t* )&load, sizeof( load ) );
		}
		if( em & MLinkThreadEvent::EthernetEvent )
			sendEthernetStatus();
		if( em & MLinkThreadEvent::GsmModemMLinkEvent )
		{
			configMutex.lock();
			sendGsmModemStatusM();
			configMutex.unlock();
		}
		if( em & MLinkThreadEvent::NetworkSharedComPortThreadFinishedEvent && networkSharedComPortThread )
		{
			delete networkSharedComPortThread;
			networkSharedComPortThread = nullptr;
			sendComPortSharingStatus();
		}
	}

	delete sensorDataChannel;
}

void MarvieDevice::uiThreadMain()
{
	/*uint32_t load = 55;
	while( true )
	{
		auto t0 = chVTGetSystemTime();
		auto t1 = t0 + TIME_MS2I( 10 * load / 100 );
		while( chVTIsSystemTimeWithin( t0, t1 ) );
		chThdSleep( TIME_MS2I( 10 * ( 100 - load ) / 100 ) );
	}*/

	thread_reference_t ref = nullptr;
	chSysLock();
	chThdSuspendS( &ref );
}

void MarvieDevice::terminalOutputThreadMain()
{
	while( true )
	{
		auto em = Thread::waitAnyEvent( ALL_EVENTS );
		if( em & TerminalOutputThreadEvent::TerminalOutputEvent )
		{
			uint32_t totalSize = terminalOutputBuffer.readAvailable();
			if( totalSize )
			{
				uint8_t* data[2];
				uint32_t dataSize[2];
				ringToLinearArrays( terminalOutputBuffer.begin(), totalSize, data, dataSize, data + 1, dataSize + 1 );
				while( dataSize[0] )
				{
					uint32_t partSize = dataSize[0];
					if( partSize > 255 )
						partSize = 255;
					mLinkServer->sendPacket( MarviePackets::Type::TerminalOutput, data[0], partSize );
					data[0] += partSize;
					dataSize[0] -= partSize;
				}
				while( dataSize[1] )
				{
					uint32_t partSize = dataSize[1];
					if( partSize > 255 )
						partSize = 255;
					mLinkServer->sendPacket( MarviePackets::Type::TerminalOutput, data[1], partSize );
					data[1] += partSize;
					dataSize[1] -= partSize;
				}
				terminalOutputBuffer.read( nullptr, totalSize );
			}
		}
	}
}

void MarvieDevice::networkServiceThreadMain()
{
	StaticTimer<> pingTimer( [this]() {
		chSysLockFromISR();
		networkServiceThread->signalEventsI( NetworkServiceThreadEvent::CheckGsmModemEvent );
		chSysUnlockFromISR();
	} );
	pingTimer.setInterval( TIME_S2I( 60 ) );

	StaticTimer<> checkNtpServerAddrTimer( [this]() {
		chSysLockFromISR();
		networkServiceThread->signalEventsI( NetworkServiceThreadEvent::CheckNtpServerAddrEvent );
		chSysUnlockFromISR();
	} );
	checkNtpServerAddrTimer.setInterval( TIME_S2I( 60 ) );

	static constexpr char ntpServerName[] = "pool.ntp.org";
	ip_addr_t ntpServerAddr = {};
	if( netconn_gethostbyname( ntpServerName, &ntpServerAddr ) != ERR_OK )
		checkNtpServerAddrTimer.start();

	MarvieBackup* backup = MarvieBackup::instance();
	volatile int n = 0;
	while( true )
	{
		eventmask_t em = Thread::waitAnyEvent( ALL_EVENTS );
		if( em & GsmModemNetworkServiceEvent )
		{
			configMutex.lock();
			volatile bool res = gsmModem && gsmModem->state() == ModemState::Working;
			configMutex.unlock();
			if( res )
			{
				pingTimer.start();
				if( checkNtpServerAddrTimer.isActive() )
					em |= CheckNtpServerAddrEvent;
			}
			else
			{
				pingTimer.stop();
				em &= ~CheckGsmModemEvent;
				Thread::getAndClearEvents( CheckGsmModemEvent );
				n = 0;
			}
		}
		if( em & CheckGsmModemEvent )
		{
			volatile bool ok = false;
			for( int i = 0; i < 8; ++i )
			{
				if( PingService::instance()->ping( testIpAddr ) )
				{
					ok = true;
					break;
				}
				/*MutexLocker locker( &configMutex );
				if( gsmModem == nullptr || gsmModem->state() != ModemState::Working )
					break;*/
			}
			if( ok )
				n = 0;
			else
			{
				if( ++n == 10 )
				{
					n = 0;
					fileLog.addRecorg( "Google is not responding" );
					mainThread->signalEvents( MainThreadEvent::RestartGsmModemRequestEvent );
				}
			}
		}
		if( em & CheckNtpServerAddrEvent )
		{
			if( netconn_gethostbyname( ntpServerName, &ntpServerAddr ) == ERR_OK )
			{
				sntp_setserver( 0, &ntpServerAddr );
				checkNtpServerAddrTimer.stop();
				Thread::getAndClearEvents( CheckNtpServerAddrEvent );

				if( backup->settings.flags.sntpClientEnabled )
					sntp_init();
			}
		}
		if( em & ConfigChangedNetworkServiceEvent )
		{
			volatile bool sntpEnabled = backup->settings.flags.sntpClientEnabled;
			if( sntpEnabled )
			{
				if( ntpServerAddr.addr != 0 && !sntp_enabled() )
					sntp_init();
			}
			else if( sntp_enabled() )
				sntp_stop();
		}
	}
}

void MarvieDevice::createGsmModemObjectM()
{
	gsmModem = new SimGsmPppModem( MarviePlatform::gsmModemEnableIoPort, false, MarviePlatform::gsmModemEnableLevel );
	gsmModem->setAsDefault();
	auto gsmUsart = comUsarts[MarviePlatform::comPortIoLines[MarviePlatform::gsmModemComPort].comUsartIndex].usart;
	configGsmModemUsart();
	gsmModem->setUsart( gsmUsart );
	gsmModem->eventSource().registerMask( &gsmModemMainThreadListener, MainThreadEvent::GsmModemMainEvent );
	gsmModem->eventSource().registerMask( &gsmModemMLinkThreadListener, MLinkThreadEvent::GsmModemMLinkEvent );
	gsmModem->eventSource().registerMask( &gsmModemNetworkServiceThreadListener, NetworkServiceThreadEvent::GsmModemNetworkServiceEvent );
	gsmModemMLinkThreadListener.setThread( mLinkServerHandlerThread->threadReference() );
	gsmModemNetworkServiceThreadListener.setThread( networkServiceThread->threadReference() );
}

void MarvieDevice::configGsmModemUsart()
{
	auto gsmUsart = comUsarts[MarviePlatform::comPortIoLines[MarviePlatform::gsmModemComPort].comUsartIndex].usart;
	if( MarviePlatform::comPortTypes[MarviePlatform::gsmModemComPort] == ComPortType::Rs485 )
		static_cast< AbstractRs485* >( comPorts[MarviePlatform::gsmModemComPort] )->disable();
	gsmUsart->setBaudRate( 230400 );
	gsmUsart->setDataFormat( UsartBasedDevice::B8N );
	gsmUsart->setStopBits( UsartBasedDevice::S1 );
}

void MarvieDevice::logFailure()
{
	auto& failureDesc = MarvieBackup::instance()->failureDesc;
	char* str = ( char* )buffer;
	if( failureDesc.type != MarvieBackup::FailureDesc::Type::None )
	{
		if( failureDesc.type == MarvieBackup::FailureDesc::Type::SystemHalt )
		{
			sprintf( str, "System was restored after a system halt [%#x %s %s]",
					 ( unsigned int )failureDesc.threadAddress, failureDesc.threadName, failureDesc.u.msg );
		}
		else
			sprintf( str, "System was restored after a hardware failure [%#x %#x %#x %#x %#x %#x %s]",
					 ( unsigned int )failureDesc.type, ( unsigned int )failureDesc.u.failure.flags,
					 ( unsigned int )failureDesc.u.failure.busAddress,
					 ( unsigned int )failureDesc.u.failure.pc,
					 ( unsigned int )failureDesc.u.failure.lr,
					 ( unsigned int )failureDesc.threadAddress, failureDesc.threadName );
		fileLog.addRecorg( str );
		failureDesc.type = MarvieBackup::FailureDesc::Type::None;
	}
	chSysLock();
	if( failureDesc.pwrDown.detected )
	{
		DateTime dateTime = failureDesc.pwrDown.dateTime;
		failureDesc.pwrDown.detected = false;
		chSysUnlock();

		char* dateStr = ( char* )buffer + 512;
		char* timeStr = ( char* )buffer + 512 + 100;
		dateTime.date().printDate( dateStr )[0] = '\0';
		dateTime.time().printTime( timeStr )[0] = '\0';
		sprintf( str, "%s at %s power failure was detected", dateStr, timeStr );
		fileLog.addRecorg( str );
	}
	else
		chSysUnlock();
}

void MarvieDevice::ejectSdCard()
{
	if( sdCardStatus != SdCardStatus::NotInserted )
	{
		sdCardStatus = SdCardStatus::NotInserted;
		if( marvieLog )
		{
			marvieLog->stopLogging();
			marvieLog->waitForStop();
		}
		removeOpenDatFiles();
		fileLog.close();
		sdcDisconnect( &SDCD1 );
	}
}

void MarvieDevice::formatSdCard()
{
	if( sdCardStatus != SdCardStatus::NotInserted && sdCardStatus != SdCardStatus::InitFailed )
	{
		sdCardStatus = SdCardStatus::Formatting;
		mLinkServerHandlerThread->signalEvents( MLinkThreadEvent::ConfigResetEvent );

		configShutdown();
		deviceState = DeviceState::IncorrectConfiguration;
		configError = ConfigError::NoConfigFile;

		removeOpenDatFiles();
		fileLog.close();
		monitoringLogSize = 0;
		if( f_mkfs( "/", FM_EXFAT, 4096, buffer, 1024 ) == FR_OK && f_mkdir( "/Temp" ) == FR_OK )
		{
			DWORD nclst;
			FATFS* fs;
			f_getfree( "0:", &nclst, &fs );
			sdCardStatus = SdCardStatus::Working;
			Dir( "/Log" ).mkdir();
			fileLog.open( "/Log/log.txt" );
		}
		else
			sdCardStatus = SdCardStatus::BadFileSystem;
		configXmlHash = SHA1::Digest();
	}
}

std::unique_ptr< File > MarvieDevice::findBootloaderFile()
{
	std::unique_ptr< File > file;
	FileInfoReader reader;
	uint8_t data[sizeof( "__MARVIE_BOOTLOADER__" ) - 1];
	while( reader.next() )
	{
		auto& info = reader.info();
		if( !info.attributes().testFlag( FileSystem::FileAttribute::ArchiveAttr ) ||
		    info.fileName().size() < sizeof( "bootloader.bin" ) - 1 ||
		    info.fileName().find( "bootloader" ) != 0 ||
		    info.fileName().find_last_of( ".bin" ) != info.fileName().size() - 1 ||
		    info.fileSize() >= 32 * 1024 || info.fileSize() < sizeof( "__MARVIE_BOOTLOADER__" ) - 1 )
			continue;

		if( !file )
			file.reset( new File( info.fileName() ) );
		if( !file )
			return std::unique_ptr< File >();
		if( !file->open( FileSystem::OpenExisting | FileSystem::Read ) ||
		    !file->seek( file->size() - sizeof( data ) ) ||
		    file->read( data, sizeof( data ) ) != sizeof( data ) )
			return std::unique_ptr< File >();
		if( strncmp( ( const char* )data, "__MARVIE_BOOTLOADER__", sizeof( data ) ) != 0 )
		{
			file->close();
			continue;
		}
		if( !file->seek( 0 ) )
			return std::unique_ptr< File >();

		return file;
	}

	return std::unique_ptr< File >();
}

void MarvieDevice::updateBootloader( File* file )
{
	firmwareTransferService.stopService();
	firmwareTransferService.waitForStopped();
	mLinkServer->stopListening();
	mLinkServer->waitForStateChanged();
	Thread::sleep( TIME_MS2I( 1000 ) );
	configShutdown();
	EthernetThread::instance()->stopThread();
	EthernetThread::instance()->waitForStop();

	FLASH_Unlock();
	uint32_t totalSize = ( uint32_t )file->size();
Start:
	file->seek( 0 );
	FLASH_EraseSector( FLASH_Sector_0, VoltageRange_3 ); // 0x08000000-0x08004000 (16 кБ) 16KB
	FLASH_EraseSector( FLASH_Sector_1, VoltageRange_3 ); // 0x08004000-0x08008000 (16 кБ) 32KB

	uint32_t size = totalSize;
	while( size )
	{
		uint32_t partSize = size;
		if( partSize > 1024 )
			partSize = 1024;
		else
		{
			for( uint32_t i = partSize; i < ( partSize + 3 ) && i < 1024; ++i )
				buffer[i] = 0xFF;
		}

		file->read( buffer, partSize );
		for( uint32_t i = 0; i < partSize; i += 4 )
		{
			FLASH_ProgramWord( 0x08000000 + totalSize - size + i, *( uint32_t* )( buffer + i ) );
			if( *( uint32_t* )( 0x08000000 + totalSize - size + i ) != *( uint32_t* )( buffer + i ) )
				goto Start;
		}
		size -= partSize;
	}
	FLASH_Lock();

	file->close();
	Dir().remove( file->fileName().c_str() );

	ejectSdCard();
	MarvieBackup::instance()->failureDesc.pwrDown.power = false;
	__NVIC_SystemReset();
}

void MarvieDevice::reconfig()
{
	auto hash = calcConfigXmlHash();
	if( hash == configXmlHash )
		return;

	if( deviceState == DeviceState::Working )
	{
		deviceState = DeviceState::Reconfiguration;
		mLinkServerHandlerThread->signalEvents( MLinkThreadEvent::ConfigResetEvent );
		configShutdown();
	}
	else
		deviceState = DeviceState::Reconfiguration;

	char* xmlData;
	uint32_t size = readConfigXmlFile( &xmlData );
	if( size )
	{
		sha.reset();
		sha.addBytes( xmlData, ( uint32_t )size );
		configXmlHash = sha.result();
		configMutex.lock();
		applyConfigM( xmlData, size );
		configMutex.unlock();
		if( deviceState == DeviceState::Working )
		{
			fileLog.addRecorg( "Configuration successfully applied" );
			mLinkServerHandlerThread->signalEvents( ( eventmask_t )MLinkThreadEvent::ConfigChangedEvent );
			networkServiceThread->signalEvents( NetworkServiceThreadEvent::ConfigChangedNetworkServiceEvent );
		}
		else
			fileLog.addRecorg( "Configuration error" );
	}
	else
	{
		configXmlHash = SHA1::Digest();
		configError = ConfigError::NoConfigFile;
		deviceState = DeviceState::IncorrectConfiguration;
		fileLog.addRecorg( "Configuration is missing" );
	}
}

void MarvieDevice::configShutdown()
{
	for( uint i = 0; i < vPortsCount; ++i )
	{
		if( brSensorReaders[i] )
			brSensorReaders[i]->stopReading();
	}
	if( marvieLog )
		marvieLog->stopLogging();
	for( uint i = 0; i < rawModbusServersCount; ++i )
		rawModbusServers[i].stopServer();
	if( tcpModbusRtuServer )
		tcpModbusRtuServer->stopServer();
	if( tcpModbusAsciiServer )
		tcpModbusAsciiServer->stopServer();
	if( tcpModbusIpServer )
		tcpModbusIpServer->stopServer();

	for( uint i = 0; i < vPortsCount; ++i )
	{
		if( brSensorReaders[i] )
			brSensorReaders[i]->waitForStateChange();
	}
	if( marvieLog )
		marvieLog->waitForStop();
	sensorLogEnabled = false;
	for( uint i = 0; i < rawModbusServersCount; ++i )
		rawModbusServers[i].waitForStateChange();
	if( tcpModbusRtuServer )
		tcpModbusRtuServer->waitForStateChange();
	if( tcpModbusAsciiServer )
		tcpModbusAsciiServer->waitForStateChange();
	if( tcpModbusIpServer )
		tcpModbusIpServer->waitForStateChange();

	removeConfigRelatedObject();
	srSensorsUpdateTimer.stop();
	mainThread->getAndClearEvents( MainThreadEvent::BrSensorReaderEvent | MainThreadEvent::SrSensorsTimerEvent );
}

void MarvieDevice::applyConfigM( char* xmlData, uint32_t len )
{
	configNum++;

	//return;
	XMLDocument* doc = new XMLDocument;
	if( doc->Parse( xmlData, len ) != XML_SUCCESS )
	{
		deviceState = DeviceState::IncorrectConfiguration;
		configError = ConfigError::XmlStructureError;

		delete xmlData;
		delete doc;
		return;
	}
	delete xmlData;

	XMLElement* rootNode = doc->FirstChildElement( MarviePlatform::configXmlRootTagName );
	if( !rootNode )
	{
		deviceState = DeviceState::IncorrectConfiguration;
		configError = ConfigError::XmlStructureError;

		delete doc;
		return;
	}
	NetworkConf networkConf;
	if( !parseNetworkConfig( rootNode->FirstChildElement( "networkConfig" ), &networkConf ) )
	{
		deviceState = DeviceState::IncorrectConfiguration;
		configError = ConfigError::NetworkConfigError;

		delete doc;
		return;
	}
	DateTimeConf dateTimeConf;
	if( !parseDateTimeConfig( rootNode->FirstChildElement( "dateTimeConfig" ), &dateTimeConf ) )
	{
		deviceState = DeviceState::IncorrectConfiguration;
		configError = ConfigError::DateTimeConfigError;

		delete doc;
		return;
	}
	SensorReadingConf sensorReadingConf;
	if( !parseSensorReadingConfig( rootNode->FirstChildElement( "sensorReadingConfig" ), &sensorReadingConf ) )
	{
		deviceState = DeviceState::IncorrectConfiguration;
		configError = ConfigError::SensorReadingConfigError;

		delete doc;
		return;
	}
	LogConf logConf;
	if( !parseLogConfig( rootNode->FirstChildElement( "logConfig" ), &logConf ) )
	{
		deviceState = DeviceState::IncorrectConfiguration;
		configError = ConfigError::LogConfigError;

		delete doc;
		return;
	}

	MarvieBackup* backup = MarvieBackup::instance();
	auto ethThd = EthernetThread::instance();
	auto ethThdConf = ethThd->currentConfig();
	if( ( networkConf.ethConf.dhcpEnable == true && ethThdConf.addressMode != EthernetThread::AddressMode::Dhcp ) ||
	    ( networkConf.ethConf.dhcpEnable == false && ethThdConf.addressMode == EthernetThread::AddressMode::Dhcp ) ||
	    networkConf.ethConf.ip != ethThdConf.ipAddress ||
	    networkConf.ethConf.netmask != ethThdConf.netmask ||
	    networkConf.ethConf.gateway != ethThdConf.gateway )
	{
		backup->acquire();
		backup->settings.flags.ethernetDhcp = networkConf.ethConf.dhcpEnable;
		backup->settings.eth.ip = networkConf.ethConf.ip;
		backup->settings.eth.netmask = networkConf.ethConf.netmask;
		backup->settings.eth.gateway = networkConf.ethConf.gateway;
		backup->settings.setValid();
		backup->release();

		ethThdConf.addressMode = networkConf.ethConf.dhcpEnable ? EthernetThread::AddressMode::Dhcp : EthernetThread::AddressMode::Static;
		ethThdConf.ipAddress = networkConf.ethConf.ip;
		ethThdConf.netmask = networkConf.ethConf.netmask;
		ethThdConf.gateway = networkConf.ethConf.gateway;
		ethThd->stopThread();
		ethThd->waitForStop();
		ethThd->setConfig( ethThdConf );
		ethThd->startThread( EthernetThreadPrio );
	}

	ComPortConf** comPortsConf = parseComPortsConfig( rootNode->FirstChildElement( "comPortsConfig" ), comPortAssignments );
	if( !comPortsConf )
	{
		deviceState = DeviceState::IncorrectConfiguration;
		configError = ConfigError::ComPortsConfigError;

		delete doc;
		return;
	}
	if( comPortsConf[MarviePlatform::gsmModemComPort]->assignment() == ComPortAssignment::GsmModem )
	{
		auto gsmConf = static_cast< GsmModemConf* >( comPortsConf[MarviePlatform::gsmModemComPort] );
		if( !gsmModem )
			createGsmModemObjectM();
		if( gsmModem->pinCode() != gsmConf->pinCode || strcmp( gsmModem->apn(), gsmConf->apn ) != 0 || !backup->settings.flags.gsmEnabled )
		{
			backup->acquire();
			backup->settings.flags.gsmEnabled = true;
			backup->settings.gsm.pinCode = gsmConf->pinCode;
			strcpy( backup->settings.gsm.apn, gsmConf->apn );
			backup->settings.setValid();
			backup->release();

			gsmModem->stopModem();
			gsmModem->waitForStateChange();
			gsmModem->setPinCode( backup->settings.gsm.pinCode );
			gsmModem->setApn( backup->settings.gsm.apn );
			if( !comPortBlockFlags[MarviePlatform::gsmModemComPort] )
				gsmModem->startModem();
		}
	}
	else if( gsmModem )
	{
		backup->acquire();
		backup->settings.flags.gsmEnabled = false;
		backup->settings.setValid();
		backup->release();

		gsmModem->stopModem();
		gsmModem->waitForStateChange();
		delete gsmModem;
		gsmModem = nullptr;
	}

	{
		if( backup->settings.flags.sntpClientEnabled != networkConf.sntpClientEnabled ||
		    backup->settings.dateTime.timeZone != dateTimeConf.timeZone )
		{
			backup->acquire();
			backup->settings.flags.sntpClientEnabled = networkConf.sntpClientEnabled;
			if( backup->settings.dateTime.timeZone != dateTimeConf.timeZone )
			{
				int64_t dt = ( dateTimeConf.timeZone - backup->settings.dateTime.timeZone ) * 3600000;
				DateTimeService::setDateTime( DateTime::fromMsecsSinceEpoch( DateTimeService::currentDateTime().msecsSinceEpoch() + dt ) );
				if( backup->settings.dateTime.lastSntpSync != -1 )
					backup->settings.dateTime.lastSntpSync += dt;
			}
			backup->settings.dateTime.timeZone = dateTimeConf.timeZone;
			backup->settings.setValid();
			backup->release();
		}
	}

	vPortsCount = 0;
	rawModbusServersCount = 0;
	enum class ModbusComPortType : uint8_t { None, Rtu, Ascii } modbusComPortTypes[MarviePlatform::comPortsCount];
	for( uint i = 0; i < MarviePlatform::comPortsCount; ++i )
	{
		auto assignment = comPortCurrentAssignments[i] = comPortsConf[i]->assignment();
		comPortServiceRoutines[i] = nullptr;
		modbusComPortTypes[i] = ModbusComPortType::None;
		if( assignment == ComPortAssignment::VPort )
		{
			++vPortsCount;
			comPorts[i]->setBaudRate( static_cast< VPortConf* >( comPortsConf[i] )->baudrate );
			comPorts[i]->setDataFormat( static_cast< VPortConf* >( comPortsConf[i] )->format );
		}
		else if( assignment == ComPortAssignment::ModbusRtuSlave || assignment == ComPortAssignment::ModbusAsciiSlave )
		{
			modbusComPortTypes[i] = assignment == ComPortAssignment::ModbusRtuSlave ? ModbusComPortType::Rtu : ModbusComPortType::Ascii;
			++rawModbusServersCount;
			comPorts[i]->setBaudRate( static_cast< VPortConf* >( comPortsConf[i] )->baudrate );
			comPorts[i]->setDataFormat( static_cast< VPortConf* >( comPortsConf[i] )->format );
		}
	}
	uint32_t ethernetVPortBegin = vPortsCount;
	vPortsCount += networkConf.vPortOverIpList.size();

	comPortServiceRoutines[MarviePlatform::gsmModemComPort] = gsmModem;

	uint16_t modbusRtuServerPort = networkConf.modbusRtuServerPort;
	uint16_t modbusAsciiServerPort = networkConf.modbusAsciiServerPort;
	uint16_t modbusIpServerPort = networkConf.modbusTcpServerPort;
	volatile bool modbusEnabled = rawModbusServersCount != 0 || networkConf.modbusTcpServerPort != 0 ||
		networkConf.modbusAsciiServerPort != 0 || networkConf.modbusRtuServerPort != 0;
	uint32_t modbusOffset = 0;

	vPorts = new IODevice*[vPortsCount];
	vPortBindings = new VPortBinding[vPortsCount];
	brSensorReaders = new BRSensorReader*[vPortsCount];
	brSensorReaderListeners = new EventListener[vPortsCount];
	for( uint i = 0; i < vPortsCount; ++i )
	{
		vPorts[i] = nullptr;
		vPortBindings[i] = VPortBinding::Rs232;
		brSensorReaders[i] = nullptr;
	}
	uint32_t n = 0;
	for( uint i = 0; i < MarviePlatform::comPortsCount; ++i )
	{
		vPortToComPortMap[i] = -1;
		if( comPortsConf[i]->assignment() == ComPortAssignment::VPort )
		{
			vPorts[n] = comPorts[i];
			vPortToComPortMap[n] = i;
			if( MarviePlatform::comPortTypes[i] == ComPortType::Rs232 )
				vPortBindings[n++] = VPortBinding::Rs232;
			else
				vPortBindings[n++] = VPortBinding::Rs485;
		}
	}
	for( uint i = 0; i < networkConf.vPortOverIpList.size(); ++i )
		vPortBindings[n++] = VPortBinding::NetworkSocket;

	sensorsCount = 0;
	sensors = nullptr;
	sensorsConfigError = SensorsConfigError::NoError;
	volatile bool srSensorsEnabled = false;
	srSensorPeriodCounter = SrSensorPeriodCounterLoad;
	errorSensorTypeName = nullptr;
	errorSensorId = 0;
	XMLElement* sensorsNode = rootNode->FirstChildElement( "sensorsConfig" );
	if( sensorsNode )
	{
		XMLElement* sensorNode = sensorsNode->FirstChildElement();
		while( sensorNode )
		{
			++sensorsCount;
			sensorNode = sensorNode->NextSiblingElement();
		}
		if( sensorsCount )
		{
			sensors = new SensorInfo[sensorsCount];
			for( uint32_t i = 0; i < sensorsCount; ++i )
			{
				sensors[i].sensor = nullptr;
				sensors[i].modbusStartRegNum = 0;
				sensors[i].vPortId = 0;
				sensors[i].logPeriod = 0;
				sensors[i].logTimeLeft = 0;
				sensors[i].dateTime = DateTime( Date( 0, 0, 0 ), Time( 0, 0, 0, 0 ) );
				sensors[i].updatedForMLink = false;
				sensors[i].readyForMLink = false;
				sensors[i].readyForLog = false;
			}
			sensorsCount = 0;
		}

		sensorNode = sensorsNode->FirstChildElement();
		while( sensorNode )
		{
			errorSensorTypeName = sensorNode->Name();
			errorSensorId = sensorsCount;
			auto desc = SensorService::instance()->sensorTypeDesc( sensorNode->Name() );
			if( !desc )
			{
				sensorsConfigError = SensorsConfigError::UnknownSensor;
				break;
			}

			if( desc->type == AbstractSensor::Type::BR )
			{
				uint32_t vPortId;
				XMLElement* c0 = sensorNode->FirstChildElement( "vPortID" );
				if( !c0 || ( vPortId = c0->UnsignedText( vPortsCount ) ) >= vPortsCount )
				{
					sensorsConfigError = SensorsConfigError::IncorrectSettings;
					break;
				}

				if( vPortsCount <= vPortId || ( vPortBindings[vPortId] != VPortBinding::Rs485 && brSensorReaders[vPortId] ) ) // FIX?
				{
					sensorsConfigError = SensorsConfigError::BingingError;
					break;
				}

				uint32_t baudrate = 0;
				if( vPortBindings[vPortId] != VPortBinding::NetworkSocket )
					baudrate = static_cast< UsartBasedDevice* >( vPorts[vPortId] )->baudRate();

				uint32_t normapPeriod;
				c0 = sensorNode->FirstChildElement( "normalPeriod" );
				if( !c0 || ( normapPeriod = c0->UnsignedText( vPortsCount ) ) > 86400 )
				{
					sensorsConfigError = SensorsConfigError::IncorrectSettings;
					break;
				}

				uint32_t emergencyPeriod = normapPeriod;
				c0 = sensorNode->FirstChildElement( "emergencyPeriod" );
				if( c0 )
					emergencyPeriod = c0->UnsignedText( emergencyPeriod );

				c0 = sensorNode->FirstChildElement( "name" );
				if( !c0 )
				{
					sensorsConfigError = SensorsConfigError::IncorrectSettings;
					break;
				}
				sensors[sensorsCount].sensorName = c0->GetText();

				AbstractSensor* sensor = desc->allocator();
				sensor->setUserData( sensorsCount );
				sensors[sensorsCount].sensor = sensor;
				sensors[sensorsCount].vPortId = vPortId;
				sensors[sensorsCount++].modbusStartRegNum = modbusOffset / 2;
				modbusOffset += sensor->sensorDataSize() + sizeof( DateTime );
				static_cast< AbstractBRSensor* >( sensor )->setIODevice( vPorts[vPortId] );
				if( !desc->tuner( sensor, sensorNode, baudrate ) )
				{
					delete sensor;
					sensorsConfigError = SensorsConfigError::IncorrectSettings;
					break;
				}

				if( vPortBindings[vPortId] == VPortBinding::Rs232 )
				{
					SingleBRSensorReader* reader = new SingleBRSensorReader;
					reader->setSensor( static_cast< AbstractBRSensor* >( sensor ), TIME_S2I( normapPeriod ), TIME_S2I( emergencyPeriod ) );
					comPortServiceRoutines[vPortToComPortMap[vPortId]] = brSensorReaders[vPortId] = reader;
				}
				else if( vPortBindings[vPortId] == VPortBinding::Rs485 )
				{
					MultipleBRSensorsReader* reader;
					if( brSensorReaders[vPortId] )
						reader = static_cast< MultipleBRSensorsReader* >( brSensorReaders[vPortId] );
					else
					{
						comPortServiceRoutines[vPortToComPortMap[vPortId]] = brSensorReaders[vPortId] = reader = new MultipleBRSensorsReader;
						reader->setMinInterval( TIME_S2I( sensorReadingConf.rs485MinInterval ) );
					}
					auto sensorElement = reader->createSensorElement( static_cast< AbstractBRSensor* >( sensor ), TIME_S2I( normapPeriod ), TIME_S2I( emergencyPeriod ) );
					reader->addSensorElement( sensorElement );
				}
				else if( vPortBindings[vPortId] == VPortBinding::NetworkSocket )
				{
					NetworkSensorReader* reader = new NetworkSensorReader;
					auto& conf = networkConf.vPortOverIpList[vPortId - ethernetVPortBegin];
					reader->setSensorAddress( IpAddress( conf.ip ), conf.port );
					reader->setSensor( static_cast< AbstractBRSensor* >( sensor ), TIME_S2I( normapPeriod ), TIME_S2I( emergencyPeriod ) );
					brSensorReaders[vPortId] = reader;
				}
				else
				{
					// TODO: add code here
				}
			}
			else // desc->type == AbstractSensor::Type::SR
			{
				srSensorsEnabled = true;

				uint32_t blockId;
				XMLElement* c0 = sensorNode->FirstChildElement( "blockID" );
				if( !c0 || ( blockId = c0->UnsignedText( 8 ) ) >= 8 )
				{
					sensorsConfigError = SensorsConfigError::IncorrectSettings;
					break;
				}

				c0 = sensorNode->FirstChildElement( "logPeriod" );
				if( !c0 || c0->QueryUnsignedText( ( unsigned int* )&sensors[sensorsCount].logPeriod ) != XML_SUCCESS )
				{
					sensorsConfigError = SensorsConfigError::IncorrectSettings;
					break;
				}
				if( sensors[sensorsCount].logPeriod < SrSensorInterval )
					sensors[sensorsCount].logPeriod = SrSensorInterval;
				sensors[sensorsCount].logPeriod -= sensors[sensorsCount].logPeriod % SrSensorInterval;

				c0 = sensorNode->FirstChildElement( "name" );
				if( !c0 )
				{
					sensorsConfigError = SensorsConfigError::IncorrectSettings;
					break;
				}
				sensors[sensorsCount].sensorName = c0->GetText();

				AbstractSensor* sensor = desc->allocator();
				sensor->setUserData( sensorsCount );
				sensors[sensorsCount].sensor = sensor;
				sensors[sensorsCount].vPortId = 0xFFFF;
				sensors[sensorsCount++].modbusStartRegNum = modbusOffset / 2;
				modbusOffset += sensor->sensorDataSize() + sizeof( DateTime );
				static_cast< AbstractSRSensor* >( sensor )->setInputSignalProvider( this );
				if( !desc->tuner( sensor, sensorNode, 0 ) )
				{
					sensorsConfigError = SensorsConfigError::IncorrectSettings;
					break;
				}

				// TODO: add code here?
			}

			sensorNode = sensorNode->NextSiblingElement();
		}
	}

	delete doc;
	for( uint i = 0; i < comPortAssignments.size(); ++i )
		delete comPortsConf[i];
	delete comPortsConf;

	if( sensorsConfigError == SensorsConfigError::NoError )
	{
		sensorLogEnabled = logConf.sensorsMode != LogConf::SensorsMode::Disabled;
		if( logConf.digitInputsMode != LogConf::DigitInputsMode::Disabled ||
			logConf.analogInputsMode != LogConf::AnalogInputsMode::Disabled ||
			logConf.sensorsMode != LogConf::SensorsMode::Disabled )
		{
			marvieLog = new MarvieLog;
			marvieLog->setRootPath( "/Log/Monitoring" );
			marvieLog->setMaxSize( logConf.maxSize );
			marvieLog->setOverwritingEnabled( logConf.overwriting );
			marvieLog->setOnlyNewDigitSignal( logConf.digitInputsMode == LogConf::DigitInputsMode::ByChange );
			if( logConf.digitInputsPeriod != 0 && logConf.digitInputsPeriod < 100 )
				logConf.digitInputsPeriod = 100;
			if( logConf.analogInputsPeriod != 0 && logConf.analogInputsPeriod < 1000 )
				logConf.analogInputsPeriod = 1000;
			marvieLog->setDigitSignalPeriod( logConf.digitInputsPeriod );
			marvieLog->setAnalogSignalPeriod( logConf.analogInputsPeriod );
			std::list< MarvieLog::SignalBlockDesc > list;
			list.push_back( MarvieLog::SignalBlockDesc{ MarviePlatform::digitInputsCount, MarviePlatform::analogInputsCount } );
			marvieLog->setSignalBlockDescList( list );
			marvieLog->setSignalProvider( this );
			marvieLog->startLogging( LOWPRIO );
		}
		else
			monitoringLogSize = Dir( "/Log/Monitoring" ).contentSize();

		if( modbusEnabled )
		{
			if( modbusOffset )
			{
				modbusRegisters = new uint16_t[modbusOffset / 2];
				memset( modbusRegisters, 0, modbusOffset );
				modbusRegistersCount = modbusOffset / 2;
			}

			rawModbusServers = new RawModbusServer[rawModbusServersCount];
			for( uint32_t iPort = 0, iServer = 0; iPort < MarviePlatform::comPortsCount; ++iPort )
			{
				if( modbusComPortTypes[iPort] != ModbusComPortType::None )
				{
					comPortServiceRoutines[iPort] = &rawModbusServers[iServer];
					rawModbusServers[iServer].setFrameType( modbusComPortTypes[iPort] == ModbusComPortType::Rtu ? ModbusDevice::FrameType::Rtu : ModbusDevice::FrameType::Ascii );
					rawModbusServers[iServer].setSlaveHandler( this );
					rawModbusServers[iServer].setIODevice( comPorts[iPort] );
					if( !comPortBlockFlags[iPort] )
						rawModbusServers[iServer++].startServer();
				}
			}

			if( modbusRtuServerPort )
			{
				tcpModbusRtuServer = new TcpModbusServer;
				tcpModbusRtuServer->setFrameType( ModbusDevice::FrameType::Rtu );
				tcpModbusRtuServer->setSlaveHandler( this );
				tcpModbusRtuServer->setPort( modbusRtuServerPort );
				tcpModbusRtuServer->setMaxClientsCount( 4 );
				tcpModbusRtuServer->startServer();
			}
			if( modbusAsciiServerPort )
			{
				tcpModbusAsciiServer = new TcpModbusServer;
				tcpModbusAsciiServer->setFrameType( ModbusDevice::FrameType::Ascii );
				tcpModbusAsciiServer->setSlaveHandler( this );
				tcpModbusAsciiServer->setPort( modbusAsciiServerPort );
				tcpModbusAsciiServer->setMaxClientsCount( 4 );
				tcpModbusAsciiServer->startServer();
			}
			if( modbusIpServerPort )
			{
				tcpModbusIpServer = new TcpModbusServer;
				tcpModbusIpServer->setFrameType( ModbusDevice::FrameType::Ip );
				tcpModbusIpServer->setSlaveHandler( this );
				tcpModbusIpServer->setPort( modbusIpServerPort );
				tcpModbusIpServer->setMaxClientsCount( 4 );
				tcpModbusIpServer->startServer();
			}
		}

		for( uint i = 0; i < vPortsCount; ++i )
		{
			brSensorReaderListeners[i].getAndClearFlags();
			if( brSensorReaders[i] )
			{
				brSensorReaders[i]->eventSource().registerMask( brSensorReaderListeners + i, MainThreadEvent::BrSensorReaderEvent );
				if( i < MarviePlatform::comPortsCount && vPortToComPortMap[i] != -1 && comPortBlockFlags[vPortToComPortMap[i]] )
					continue;
				brSensorReaders[i]->startReading( NORMALPRIO );
			}
		}

		if( srSensorsEnabled )
		{
			srSensorsUpdateTimer.setParameter( MainThreadEvent::SrSensorsTimerEvent );
			srSensorsUpdateTimer.start( TIME_MS2I( SrSensorInterval ) );
		}

		deviceState = DeviceState::Working;
	}
	else
	{
		removeConfigRelatedObjectM();
		deviceState = DeviceState::IncorrectConfiguration;
	}
}

void MarvieDevice::removeConfigRelatedObject()
{
	configMutex.lock();
	removeConfigRelatedObjectM();
	configMutex.unlock();
}

void MarvieDevice::removeConfigRelatedObjectM()
{
	for( uint i = 0; i < MarviePlatform::comPortsCount; ++i )
	{
		if( comPortCurrentAssignments[i] != MarvieXmlConfigParsers::ComPortAssignment::GsmModem )
		{
			comPortCurrentAssignments[i] = MarvieXmlConfigParsers::ComPortAssignment::VPort;
			comPortServiceRoutines[i] = nullptr;
			vPortToComPortMap[i] = -1;
		}
	}
	for( uint i = 0; i < vPortsCount; ++i )
		delete brSensorReaders[i];
	for( uint i = 0; i < sensorsCount; ++i )
		delete sensors[i].sensor;

	delete vPorts;
	delete vPortBindings;
	delete[] sensors;
	delete brSensorReaders;
	delete[] brSensorReaderListeners;

	delete marvieLog;

	delete[] rawModbusServers;
	delete tcpModbusRtuServer;
	delete tcpModbusAsciiServer;
	delete tcpModbusIpServer;
	delete modbusRegisters;

	vPortsCount = 0;
	vPorts = nullptr;
	vPortBindings = nullptr;
	sensorsCount = 0;
	sensors = nullptr;
	brSensorReaders = nullptr;
	brSensorReaderListeners = nullptr;

	marvieLog = nullptr;

	rawModbusServers = nullptr;
	rawModbusServersCount = 0;
	tcpModbusRtuServer = nullptr;
	tcpModbusAsciiServer = nullptr;
	tcpModbusIpServer = nullptr;
	modbusRegisters = nullptr;
	modbusRegistersCount = 0;
}

UsartBasedDevice* MarvieDevice::startComPortSharing( uint32_t index )
{
	if( index >= MarviePlatform::comPortsCount )
		return nullptr;
	MutexLocker locker( sharingComPortMutex );
	CriticalSectionLocker criticalSectionLocker;
	if( sharedComPortIndex != -1 )
		return nullptr;
	sharedComPortIndex = index;
	mainThread->signalEventsI( MainThreadEvent::StartComPortSharingEvent );
	sharingComPortThreadQueue.enqueueSelf( TIME_INFINITE );
	return comPorts[sharedComPortIndex];
}

void MarvieDevice::stopComPortSharing()
{
	MutexLocker locker( sharingComPortMutex );
	CriticalSectionLocker criticalSectionLocker;
	if( sharedComPortIndex == -1 )
		return;
	mainThread->signalEventsI( MainThreadEvent::StopComPortSharingEvent );
	sharingComPortThreadQueue.enqueueSelf( TIME_INFINITE );
	sharedComPortIndex = -1;
}

void MarvieDevice::networkSharedComPortThreadMain( UsartBasedDevice* ioDevice, MarviePackets::ComPortSharingSettings::Mode mode )
{
	enum Event : eventmask_t
	{
		IODeviceEvent = 2,
		ServerEvent = 4,
		SocketEvent = 8,
		TimeoutEvent = 16
	};
	enum class State
	{
		WaitHeader,
		WaitPayload
	} state = State::WaitHeader;
	uint32_t blockSize = 0;
	uint8_t buffer[128];
	EventListener ioDeviceListener, serverListener, socketListener;
	TcpServer server;
	TcpSocket* socket = nullptr;
	volatile bool needSignal = true;
	StaticTimer<> timer( []( ThreadRef ref ) {
		chSysLockFromISR();
		ref.signalEventsI( Event::TimeoutEvent );
		chSysUnlockFromISR();
	}, ThreadRef::currentThread() );
	timer.setInterval( TIME_S2I( 60 * 60 ) );
	timer.setSingleShot( true );

	ioDevice->eventSource().registerMaskWithFlags( &ioDeviceListener, IODeviceEvent, CHN_INPUT_AVAILABLE );
	server.eventSource().registerMask( &serverListener, ServerEvent );
	if( !server.listen( 14789 ) )
		goto End;
	ioDevice->read( nullptr, ioDevice->readAvailable() );

	chSysLock();
	sharedComPortNetworkClientsCount = 0;
	chSysUnlock();
	timer.start();
	while( true )
	{
		eventmask_t em = Thread::waitAnyEvent( ALL_EVENTS );
		if( em & StopNetworkSharedComPortThreadRequestEvent )
		{
			needSignal = false;
			break;
		}
		if( em & TimeoutEvent )
			break;
		if( em & IODeviceEvent )
		{
			if( socket )
			{
				uint32_t size = ioDevice->readAvailable();
				while( size )
				{
					uint32_t partSize = size;
					if( partSize > sizeof( buffer ) )
						partSize = sizeof( buffer );
					ioDevice->read( buffer, partSize );
					socket->write( buffer, partSize );
					size -= partSize;
				}
			}
			else
				ioDevice->read( nullptr, ioDevice->readAvailable() );
		}
		if( em & ServerEvent )
		{
			if( !server.isListening() )
				break;
			TcpSocket* newSocket;
			while( ( newSocket = server.nextPendingConnection() ) )
			{
				if( socket )
				{
					newSocket->disconnect();
					delete newSocket;
				}
				else
				{
					socket = newSocket;
					socket->eventSource().registerMask( &socketListener, SocketEvent );
					em |= SocketEvent;
					chSysLock();
					sharedComPortNetworkClientsCount = 1;
					chSysUnlock();
				}
			}
		}
		if( em & SocketEvent && socket )
		{
			if( !socket->isOpen() )
			{
				delete socket;
				socket = nullptr;
				chSysLock();
				sharedComPortNetworkClientsCount = 0;
				chSysUnlock();
			}
			else
			{
				timer.start();
				while( socket->readAvailable() )
				{
					if( mode == MarviePackets::ComPortSharingSettings::Mode::BlockStream )
					{
						if( state == State::WaitHeader )
						{
							uint16_t size;
							socket->read( ( uint8_t* )&size, sizeof( size ) );
							blockSize = size;
							state = State::WaitPayload;
						}

						if( socket->readAvailable() < blockSize )
							break;

						while( blockSize )
						{
							uint32_t partSize = blockSize;
							if( partSize > sizeof( buffer ) )
								partSize = sizeof( buffer );
							socket->read( buffer, partSize );
							ioDevice->write( buffer, partSize );
							blockSize -= partSize;
						}
						state = State::WaitHeader;
					}
					else
					{
						blockSize = socket->readAvailable();
						while( blockSize )
						{
							uint32_t partSize = blockSize;
							if( partSize > sizeof( buffer ) )
								partSize = sizeof( buffer );
							socket->read( buffer, partSize );
							ioDevice->write( buffer, partSize );
							blockSize -= partSize;
						}
					}
				}
			}
		}
	}
	server.close();
	if( socket )
		delete socket;
End:
	chSysLock();
	stopComPortSharing();
	sharedComPortNetworkClientsCount = -1;
	if( needSignal )
		mLinkServerHandlerThread->signalEventsI( MLinkThreadEvent::NetworkSharedComPortThreadFinishedEvent );
}

const char* MarvieDevice::firmwareVersion()
{
	return MarviePlatform::coreVersion;
}

const char* MarvieDevice::bootloaderVersion()
{
	return _bootloaderVersion;
}

void MarvieDevice::firmwareDownloaded( const std::string& fileName )
{
	datFilesMutex.lock();
	Dir dir;
	dir.remove( "/firmware.bin" );
	dir.rename( fileName.c_str(), "/firmware.bin" );
	datFilesMutex.unlock();
}

void MarvieDevice::bootloaderDownloaded( const std::string& fileName )
{
	datFilesMutex.lock();
	Dir dir;
	dir.remove( "/bootloader.bin" );
	dir.rename( fileName.c_str(), "/bootloader.bin" );
	datFilesMutex.unlock();
}

void MarvieDevice::restartDevice()
{
	mainThread->signalEvents( MainThreadEvent::RestartRequestEvent );
}

void MarvieDevice::copySensorDataToModbusRegisters( AbstractSensor* sensor )
{
	if( modbusRegisters )
	{
		modbusRegistersMutex.lock();
		sensor->sensorData()->lock();
		if( sensor->sensorData()->isValid() )
		{
			uint32_t count = sensor->sensorDataSize() / 2;
			uint16_t* reg = modbusRegisters + sensors[sensor->userData()].modbusStartRegNum;
			*( DateTime* )reg = sensor->sensorData()->time();
			reg += sizeof( DateTime ) / 2;
			uint16_t* data = ( uint16_t* )( ( uint8_t* )sensor->sensorData() + sizeof( SensorData ) );
			for( uint32_t i = 0; i < count; ++i )
				reg[i] = data[i];
		}
		sensor->sensorData()->unlock();
		modbusRegistersMutex.unlock();
	}
}

float MarvieDevice::analogSignal( uint32_t block, uint32_t line )
{
	if( block == 0 )
	{
		if( line < MarviePlatform::analogInputsCount )
			return analogInput[line];
		return -1;
	}

	return -1;
}

bool MarvieDevice::digitSignal( uint32_t block, uint32_t line )
{
	if( block == 0 )
	{
		if( line < MarviePlatform::digitInputsCount )
			return digitInputs & ( 1 << line );
		return false;
	}

	return false;
}

uint32_t MarvieDevice::digitSignals( uint32_t block )
{
	if( block == 0 )
		return digitInputs;
	return 0;
}

int MarvieDevice::authenticate( char* accountName, char* password )
{
	MarvieBackup* backup = MarvieBackup::instance();
	if( strcmp( accountName, "__system" ) == 0 )
	{
		if( strcmp( password, backup->settings.passwords.adminPassword ) == 0 )
			return 0;
	}
	if( strcmp( accountName, "admin" ) == 0 )
	{
		if( strcmp( password, backup->settings.passwords.adminPassword ) == 0 )
			return 1;
	}
	else if( strcmp( accountName, "observer" ) == 0 )
	{
		if( strcmp( password, backup->settings.passwords.observerPassword ) == 0 )
			return 2;
	}

	return -1;
}

bool MarvieDevice::onOpennig( uint8_t ch, const char* name, uint32_t size )
{
	datFilesMutex.lock();
	if( sdCardStatus != SdCardStatus::Working || ch >= 3 || datFiles[ch] || mLinkServer->accountIndex() > 1 )
	{
		datFilesMutex.unlock();
		if( mLinkServer->accountIndex() > 1 )
			mLinkServer->sendPacket( MarviePackets::Type::IllegalAccessType, nullptr, 0 );
		return false;
	}

	char path[] = "/Temp/0.dat";
	path[6] = '0' + ch;
	datFiles[ch] = new FIL;
	if( f_open( datFiles[ch], path, FA_OPEN_ALWAYS | FA_WRITE | FA_READ ) != FR_OK )
	{
		delete datFiles[ch];
		datFiles[ch] = nullptr;

		datFilesMutex.unlock();
		return false;
	}

	datFilesMutex.unlock();
	return true;
}

bool MarvieDevice::newDataReceived( uint8_t ch, const uint8_t* data, uint32_t size )
{
	datFilesMutex.lock();
	if( sdCardStatus != SdCardStatus::Working || ch >= 3 || datFiles[ch] == nullptr )
		goto Abort;

	uint bw;
	if( f_write( datFiles[ch], data, size, &bw ) != FR_OK || bw != size )
		goto Abort;

	datFilesMutex.unlock();
	return true;

Abort:
	datFilesMutex.unlock();
	return false;
}

void MarvieDevice::onClosing( uint8_t ch, bool canceled )
{
	datFilesMutex.lock();
	if( sdCardStatus != SdCardStatus::Working || ch >= 3 || datFiles[ch] == nullptr )
	{
		datFilesMutex.unlock();
		return;
	}

	if( canceled )
	{
		char path[] = "/Temp/0.dat";
		path[6] = '0' + ch;
		f_close( datFiles[ch] );
		f_unlink( path );
		delete datFiles[ch];
		datFiles[ch] = nullptr;
	}
	else
	{
		//f_sync( datFiles[id] );
		switch( ch )
		{
		case MarviePackets::ComplexChannel::BootloaderChannel:
			mainThread->signalEvents( MainThreadEvent::NewBootloaderDatFile );
			break;
		case MarviePackets::ComplexChannel::FirmwareChannel:
			mainThread->signalEvents( MainThreadEvent::NewFirmwareDatFile );
			break;
		case MarviePackets::ComplexChannel::XmlConfigChannel:
			mainThread->signalEvents( MainThreadEvent::NewXmlConfigDatFile );
			break;
		default:
			break;
		}
	}
	datFilesMutex.unlock();
}


void MarvieDevice::terminalOutput( const uint8_t* data, uint32_t size, void* p )
{
	MarvieDevice* marvieDevice = reinterpret_cast< MarvieDevice* >( p );
	CriticalSectionLocker locker;
	while( size )
	{
		if( marvieDevice->mLinkServer->state() != MLinkServer::State::Connected )
			return;
		uint32_t partSize = size;
		if( partSize > marvieDevice->terminalOutputBuffer.capacity() )
			partSize = marvieDevice->terminalOutputBuffer.capacity();
		if( marvieDevice->terminalOutputBuffer.writeAvailable() <= partSize )
		{
			marvieDevice->terminalOutputThread->signalEventsI( TerminalOutputThreadEvent::TerminalOutputEvent );
			marvieDevice->terminalOutputTimer.stop();
		}
		else
			marvieDevice->terminalOutputTimer.start( TIME_MS2I( 1 ) );
		marvieDevice->terminalOutputBuffer.write( data, partSize, TIME_INFINITE );
		data += partSize;
		size -= partSize;
	}
}

void MarvieDevice::mLinkProcessNewPacket( uint32_t type, uint8_t* data, uint32_t size )
{
	if( type == MarviePackets::Type::GetConfigXmlType )
	{
		if( !configXmlDataSendingSemaphore.acquire( TIME_IMMEDIATE ) )
			return;

		char* xmlData = nullptr;
		uint32_t xmlDataSize = readConfigXmlFile( &xmlData );
		if( xmlData )
		{
			auto channel = mLinkServer->createDataChannel();
			if( channel->open( MarviePackets::ComplexChannel::XmlConfigChannel, "", xmlDataSize ) )
			{
				Concurrent::run( ThreadProperties( 1024, NORMALPRIO ), [this, channel, xmlData, xmlDataSize]()
				{
					channel->sendData( ( uint8_t* )xmlData, xmlDataSize );
					channel->close();
					delete xmlData;
					delete channel;
					configXmlDataSendingSemaphore.release();
				} );
			}
		}
		else
		{
			configXmlDataSendingSemaphore.release();
			mLinkServer->sendPacket( MarviePackets::Type::ConfigXmlMissingType, nullptr, 0 );
		}
	}
	else
	{
		if( mLinkServer->accountIndex() > 1 )
		{
			if( type != MarviePackets::Type::TerminalInput )
				mLinkServer->sendPacket( MarviePackets::Type::IllegalAccessType, nullptr, 0 );
			return;
		}

		if( type == MarviePackets::Type::TerminalInput )
		{
			terminalServer.input( data, size );
		}
		else if( type == MarviePackets::Type::ChangeAccountPasswordType )
		{
			auto newPassword = reinterpret_cast< MarviePackets::AccountNewPassword* >( data );
			int index = authenticate( newPassword->name, newPassword->currentPassword );
			if( index == -1 )
			{
				uint8_t err = 1;
				mLinkServer->sendPacket( MarviePackets::Type::ChangeAccountPasswordResultType, &err, sizeof( err ) );
			}
			else
			{
				MarvieBackup* backup = MarvieBackup::instance();
				backup->acquire();
				if( index == 0 || index == 1 )
					strcpy( backup->settings.passwords.adminPassword, newPassword->newPassword );
				else
					strcpy( backup->settings.passwords.observerPassword, newPassword->newPassword );
				backup->settings.setValid();
				backup->release();
				uint8_t err = 0;
				mLinkServer->sendPacket( MarviePackets::Type::ChangeAccountPasswordResultType, &err, sizeof( err ) );
			}
		}
		else if( type == MarviePackets::Type::SetDateTimeType )
		{
			DateTime dateTime;
			memcpy( ( char* )&dateTime, ( const char* )data, sizeof( DateTime ) );
			dateTime = DateTime::fromMsecsSinceEpoch( dateTime.msecsSinceEpoch() + MarvieBackup::instance()->settings.dateTime.timeZone * 3600000 );

			MarvieBackup* backup = MarvieBackup::instance();
			backup->acquire();
			backup->settings.dateTime.lastSntpSync = -1;
			backup->settings.setValid();
			DateTimeService::setDateTime( dateTime );
			backup->release();
		}
		else if( type == MarviePackets::Type::StartVPortsType )
		{
			mainThread->signalEvents( MainThreadEvent::StartSensorReaders );
		}
		else if( type == MarviePackets::Type::StopVPortsType )
		{
			mainThread->signalEvents( MainThreadEvent::StopSensorReaders );
		}
		else if( type == MarviePackets::Type::UpdateAllSensorsType )
		{
			configMutex.lock();
			for( uint i = 0; i < vPortsCount; ++i )
			{
				if( brSensorReaders[i] )
					brSensorReaders[i]->forceAll();
			}
			configMutex.unlock();
		}
		else if( type == MarviePackets::Type::UpdateOneSensorType )
		{
			configMutex.lock();
			uint32_t sensorId = *( uint16_t* )data;
			if( sensorsCount > sensorId && sensors[sensorId].sensor->type() == AbstractSensor::Type::BR )
			{
				auto reader = brSensorReaders[sensors[sensorId].vPortId];
				if( reader )
					reader->forceOne( reinterpret_cast< AbstractBRSensor* >( sensors[sensorId].sensor ) );
			}
			configMutex.unlock();
		}
		else if( type == MarviePackets::Type::RestartDeviceType )
		    mainThread->signalEvents( MainThreadEvent::RestartRequestEvent );
		else if( type == MarviePackets::Type::EjectSdCardType )
		{
			if( sdCardStatus != SdCardStatus::NotInserted )
				mainThread->signalEvents( MainThreadEvent::EjectSdCardRequest );
		}
		else if( type == MarviePackets::Type::CleanMonitoringLogType )
		    mainThread->signalEvents( MainThreadEvent::CleanMonitoringLogRequestEvent );
		else if( type == MarviePackets::Type::CleanSystemLogType )
		    mainThread->signalEvents( MainThreadEvent::CleanSystemLogRequestEvent );
		else if( type == MarviePackets::Type::FormatSdCardType )
		{
			if( sdCardStatus != SdCardStatus::Formatting )
				mainThread->signalEvents( MainThreadEvent::FormatSdCardRequest );
		}
		else if( type == MarviePackets::Type::StartComPortSharingType )
		{
			if( networkSharedComPortThread )
			{
				uint8_t result = 1;
				mLinkServer->sendPacket( MarviePackets::Type::StartComPortSharingResultType, &result, sizeof( result ) );
				return;
			}
			MarviePackets::ComPortSharingSettings* packet = reinterpret_cast< MarviePackets::ComPortSharingSettings* >( data );
			auto io = startComPortSharing( packet->comPortIndex );
			if( io == nullptr )
			{
				uint8_t result = 1;
				mLinkServer->sendPacket( MarviePackets::Type::StartComPortSharingResultType, &result, sizeof( result ) );
				return;
			}
			io->setDataFormat( UsartBasedDevice::DataFormat( packet->format ) );
			io->setStopBits( UsartBasedDevice::StopBits( packet->stopBits ) );
			io->setBaudRate( packet->baudrate );
			networkSharedComPortThread = Thread::createAndStart( ThreadProperties( 1536 + 128, NORMALPRIO ), [this]( UsartBasedDevice* ioDevice, MarviePackets::ComPortSharingSettings::Mode mode ) {
				return networkSharedComPortThreadMain( ioDevice, mode );
			}, std::move( io ), std::move( packet->mode ) );
			if( !networkSharedComPortThread )
			{
				uint8_t result = 1;
				mLinkServer->sendPacket( MarviePackets::Type::StartComPortSharingResultType, &result, sizeof( result ) );
				return;
			}
			uint8_t result = 0;
			mLinkServer->sendPacket( MarviePackets::Type::StartComPortSharingResultType, &result, sizeof( result ) );
		}
		else if( type == MarviePackets::Type::StopComPortSharingType )
		{
			if( !networkSharedComPortThread )
				return;

			networkSharedComPortThread->signalEvents( StopNetworkSharedComPortThreadRequestEvent );
			networkSharedComPortThread->wait();
			delete networkSharedComPortThread;
			networkSharedComPortThread = nullptr;
		}
	}
}

void MarvieDevice::mLinkSync( bool coldSync )
{
	mLinkServer->sendPacket( MarviePackets::Type::SyncStartType, nullptr, 0 );
	if( coldSync )
	{
		sendFirmwareDesc();
		sendSupportedSensorsList();
		sendDeviceSpec();
	}

	sendDeviceStatus();
	sendEthernetStatus();
	sendComPortSharingStatus();
	configMutex.lock();
	sendGsmModemStatusM();
	sendServiceStatisticsM();
	configMutex.unlock();
	if( mLinkServer->accountIndex() == 0 )
	{
		mLinkServer->sendPacket( MarviePackets::Type::SyncEndType, nullptr, 0 );
		return;
	}

	auto channel = mLinkServer->createDataChannel();
	syncConfigNum = configNum;

	// sending config.xml
	char* xmlData = nullptr;
	uint32_t xmlDataSize = readConfigXmlFile( &xmlData );
	if( xmlDataSize )
	{
		configXmlDataSendingSemaphore.acquire( TIME_INFINITE );
		if( channel->open( MarviePackets::ComplexChannel::XmlConfigSyncChannel, "", xmlDataSize ) )
		{
			channel->sendData( ( uint8_t* )xmlData, xmlDataSize );
			channel->close();
		}
		delete xmlData;
		configXmlDataSendingSemaphore.release();
	}

	sendAnalogInputsData();
	sendDigitInputsData();

	configMutex.lock();
	if( syncConfigNum == configNum )
	{
		mLinkServer->sendPacket( MarviePackets::Type::VPortCountType, ( uint8_t* )&vPortsCount, sizeof( vPortsCount ) );
		for( uint i = 0; i < vPortsCount; ++i )
			sendVPortStatusM( i );
		for( uint i = 0; i < sensorsCount; ++i )
			sendSensorDataM( i, channel );
	}
	configMutex.unlock();

	delete channel;
	mLinkServer->sendPacket( MarviePackets::Type::SyncEndType, nullptr, 0 );
}

void MarvieDevice::sendFirmwareDesc()
{
	MarviePackets::FirmwareDesc desc;
	strcpy( desc.bootloaderVersion, _bootloaderVersion );
	strcpy( desc.firmwareVersion, MarviePlatform::coreVersion );
	strcpy( desc.modelName, MarviePlatform::platformType );
	mLinkServer->sendPacket( MarviePackets::Type::FirmwareDescType, ( uint8_t* )&desc, sizeof( desc ) );
}

void MarvieDevice::sendSupportedSensorsList()
{
	auto channel = mLinkServer->createDataChannel();
	if( channel->open( MarviePackets::ComplexChannel::SupportedSensorsListChannel, "", 0 ) )
	{
		char* data = new char[1024];
		uint32_t n = 0;
		for( auto i = SensorService::beginSensorsList(); i != SensorService::endSensorsList(); ++i )
		{
			uint32_t len = strlen( ( *i ).name );
			if( 1024 - n < len )
				channel->sendData( ( uint8_t* )data, n ), n = 0;
			memcpy( data + n, ( *i ).name, len );
			n += len;
			if( i + 1 != SensorService::endSensorsList() )
			{
				if( n == 1024 )
					channel->sendData( ( uint8_t* )data, n ), n = 0;
				data[n++] = ',';
			}
		}
		channel->sendData( ( uint8_t* )data, n );
		channel->close();
		delete data;
	}
	delete channel;
}

void MarvieDevice::sendDeviceSpec()
{
	MarviePackets::DeviceSpecs specs;
	specs.comPortsCount = MarviePlatform::comPortsCount;
	mLinkServer->sendPacket( MarviePackets::Type::DeviceSpecsType, ( uint8_t* )&specs, sizeof( specs ) );
}

void MarvieDevice::sendVPortStatusM( uint32_t vPortId )
{
	if( !brSensorReaders[vPortId] )
		return;
	MarviePackets::VPortStatus status;
	status.vPortId = ( uint8_t )vPortId;
	chSysLock();
	auto s = brSensorReaders[vPortId]->state();
	if( s == BRSensorReader::State::Stopped )
		status.state = MarviePackets::VPortStatus::State::Stopped;
	else if( s == BRSensorReader::State::Working )
		status.state = MarviePackets::VPortStatus::State::Working;
	else if( s == BRSensorReader::State::Stopping )
		status.state = MarviePackets::VPortStatus::State::Stopping;
	if( brSensorReaders[vPortId]->nextSensor() )
		status.sensorId = brSensorReaders[vPortId]->nextSensor()->userData();
	else
		status.sensorId = -1;
	status.timeLeft = TIME_I2S( brSensorReaders[vPortId]->timeToNextReading() );
	chSysUnlock();
	mLinkServer->sendPacket( MarviePackets::Type::VPortStatusType, ( const uint8_t* )&status, sizeof( status ) );
}

void MarvieDevice::sendSensorDataM( uint32_t sensorId, MLinkServer::DataChannel* channel )
{
	auto sensorData = sensors[sensorId].sensor->sensorData();
	sensorData->lock();
	if( sensorData->isValid() )
	{
		if( sensorData->time().date().year() > 0 && channel->open( MarviePackets::ComplexChannel::SensorDataChannel, nullptr, 0 ) )
		{
			uint8_t header[sizeof( uint8_t ) + sizeof( uint8_t ) + sizeof( DateTime )];
			header[0] = ( uint8_t )sensorId;
			header[1] = ( uint8_t )sensors[sensorId].vPortId;
			*reinterpret_cast< DateTime* >( header + sizeof( uint8_t ) + sizeof( uint8_t ) ) = sensorData->time();
			channel->sendData( header, sizeof( header ) );
			channel->sendData( ( const uint8_t* )sensorData + sizeof( SensorData ), sensors[sensorId].sensor->sensorDataSize() );
			channel->close();
		}
	}
	else if( sensors[sensorId].sensor->type() != AbstractSensor::Type::SR )
	{
		MarviePackets::SensorErrorReport report;
		report.sensorId = ( uint16_t )sensorId;
		report.vPortId = sensors[sensorId].vPortId;
		switch( sensorData->error() )
		{
		case SensorData::Error::NoResponseError:
			report.error = MarviePackets::SensorErrorReport::Error::NoResponseError;
			break;
		case SensorData::Error::CrcError:
			report.error = MarviePackets::SensorErrorReport::Error::CrcError;
			break;
		default:
			break;
		}
		report.errorCode = sensorData->errorCode();
		report.dateTime = sensors[sensorId].dateTime;
		mLinkServer->sendPacket( MarviePackets::Type::SensorErrorReportType, ( const uint8_t* )&report, sizeof( report ) );
	}
	sensorData->unlock();
}

void MarvieDevice::sendDeviceStatus()
{
	MarviePackets::DeviceStatus status;
	status.dateTime = DateTimeService::currentDateTime();
	status.workingTime = startDateTime.msecsTo( status.dateTime ) / 1000;
	chSysLock();
	switch( deviceState )
	{
	case MarvieDevice::DeviceState::Reconfiguration:
		status.state = MarviePackets::DeviceStatus::DeviceState::Reconfiguration;
		break;
	case MarvieDevice::DeviceState::Working:
		status.state = MarviePackets::DeviceStatus::DeviceState::Working;
		break;
	case MarvieDevice::DeviceState::IncorrectConfiguration:
		status.state = MarviePackets::DeviceStatus::DeviceState::IncorrectConfiguration;
		break;
	default:
		break;
	}
	status.errorSensorId = 0;
	switch( configError )
	{
	case MarvieDevice::ConfigError::NoError:
		status.configError = MarviePackets::DeviceStatus::ConfigError::NoError;
		break;
	case MarvieDevice::ConfigError::NoConfigFile:
		status.configError = MarviePackets::DeviceStatus::ConfigError::NoConfiglFile;
		break;
	case MarvieDevice::ConfigError::XmlStructureError:
		status.configError = MarviePackets::DeviceStatus::ConfigError::XmlStructureError;
		break;
	case MarvieDevice::ConfigError::ComPortsConfigError:
		status.configError = MarviePackets::DeviceStatus::ConfigError::ComPortsConfigError;
		break;
	case MarvieDevice::ConfigError::NetworkConfigError:
		status.configError = MarviePackets::DeviceStatus::ConfigError::NetworkConfigError;
		break;
	case MarvieDevice::ConfigError::DateTimeConfigError:
		status.configError = MarviePackets::DeviceStatus::ConfigError::DateTimeConfigError;
		break;
	case MarvieDevice::ConfigError::SensorReadingConfigError:
		status.configError = MarviePackets::DeviceStatus::ConfigError::SensorReadingConfigError;
		break;
	case MarvieDevice::ConfigError::LogConfigError:
		status.configError = MarviePackets::DeviceStatus::ConfigError::LogConfigError;
		break;
	case MarvieDevice::ConfigError::SensorsConfigError:
		status.configError = MarviePackets::DeviceStatus::ConfigError::SensorsConfigError;
		status.errorSensorId = errorSensorId;
		break;
	default:
		break;
	}
	switch( sdCardStatus )
	{
	case MarvieDevice::SdCardStatus::NotInserted:
		status.sdCardStatus = MarviePackets::DeviceStatus::SdCardStatus::NotInserted;
		break;
	case MarvieDevice::SdCardStatus::Initialization:
		status.sdCardStatus = MarviePackets::DeviceStatus::SdCardStatus::Initialization;
		break;
	case MarvieDevice::SdCardStatus::InitFailed:
		status.sdCardStatus = MarviePackets::DeviceStatus::SdCardStatus::InitFailed;
		break;
	case MarvieDevice::SdCardStatus::BadFileSystem:
		status.sdCardStatus = MarviePackets::DeviceStatus::SdCardStatus::BadFileSystem;
		break;
	case MarvieDevice::SdCardStatus::Formatting:
		status.sdCardStatus = MarviePackets::DeviceStatus::SdCardStatus::Formatting;
		break;
	case MarvieDevice::SdCardStatus::Working:
		status.sdCardStatus = MarviePackets::DeviceStatus::SdCardStatus::Working;
		break;
	default:
		break;
	}

	if( marvieLog )
	{
		if( marvieLog->state() == MarvieLog::State::Working )
			status.logState = MarviePackets::DeviceStatus::LogState::Working;
		else if( marvieLog->state() == MarvieLog::State::Stopped )
			status.logState = MarviePackets::DeviceStatus::LogState::Stopped;
		else if( marvieLog->state() == MarvieLog::State::Stopping )
			status.logState = MarviePackets::DeviceStatus::LogState::Stopping;
		else if( marvieLog->state() == MarvieLog::State::Archiving )
			status.logState = MarviePackets::DeviceStatus::LogState::Archiving;
	}
	else
		status.logState = MarviePackets::DeviceStatus::LogState::Off;

	chSysUnlock();
	mLinkServer->sendPacket( MarviePackets::Type::DeviceStatusType, ( uint8_t* )&status, sizeof( status ) );
}

void MarvieDevice::sendEthernetStatus()
{
	auto eth = EthernetThread::instance();
	MarviePackets::EthernetStatus status;
	chSysLock();
	if( EthernetThread::instance()->linkStatus() == EthernetThread::LinkStatus::Up )
	{
		status.ip = *( uint32_t* )eth->networkAddress().addr;
		status.netmask = eth->networkMask();
		status.gateway = *( uint32_t* )eth->networkGateway().addr;
	}
	else
	{
		status.ip = 0;
		status.netmask = 0;
		status.gateway = 0;
	}
	chSysUnlock();
	mLinkServer->sendPacket( MarviePackets::Type::EthernetStatusType, ( uint8_t* )&status, sizeof( status ) );
}

void MarvieDevice::sendGsmModemStatusM()
{
	MarviePackets::GsmStatus status;
	status.ip = 0;
	if( gsmModem )
	{
		chSysLock();
		auto state = gsmModem->state();
		switch( state )
		{
		case ModemState::Stopped:
			status.state = MarviePackets::GsmStatus::State::Stopped;
			break;
		case ModemState::Initializing:
			status.state = MarviePackets::GsmStatus::State::Initializing;
			break;
		case ModemState::Working:
			status.state = MarviePackets::GsmStatus::State::Working;
			status.ip = *( uint32_t* )gsmModem->networkAddress().addr;
			break;
		case ModemState::Stopping:
			status.state = MarviePackets::GsmStatus::State::Stopping;
			break;
		default:
			break;
		}
		auto error = gsmModem->error();
		switch( error )
		{
		case ModemError::NoError:
			status.error = MarviePackets::GsmStatus::Error::NoError;
			break;
		case ModemError::AuthenticationError:
			status.error = MarviePackets::GsmStatus::Error::AuthenticationError;
			break;
		case ModemError::TimeoutError:
			status.error = MarviePackets::GsmStatus::Error::TimeoutError;
			break;
		case ModemError::UnknownError:
			status.error = MarviePackets::GsmStatus::Error::UnknownError;
			break;
		default:
			break;
		}
		chSysUnlock();
	}
	else
	{
		status.state = MarviePackets::GsmStatus::State::Off;
		status.error = MarviePackets::GsmStatus::Error::NoError;
	}
	mLinkServer->sendPacket( MarviePackets::Type::GsmStatusType, ( uint8_t* )&status, sizeof( status ) );
}

void MarvieDevice::sendServiceStatisticsM()
{
	MarvieBackup* backup = MarvieBackup::instance();
	MarviePackets::ServiceStatistics status;
	if( sntp_enabled() )
		status.sntpClientState = MarviePackets::ServiceStatistics::SntpState::Working;
	else if( backup->settings.flags.sntpClientEnabled )
		status.sntpClientState = MarviePackets::ServiceStatistics::SntpState::Initializing;
	else
		status.sntpClientState = MarviePackets::ServiceStatistics::SntpState::Off;
	status.rawModbusClientsCount = ( int16_t )rawModbusServersCount;
	if( tcpModbusRtuServer )
		status.tcpModbusRtuClientsCount = ( int16_t )tcpModbusRtuServer->currentClientsCount();
	else
		status.tcpModbusRtuClientsCount = -1;
	if( tcpModbusAsciiServer )
		status.tcpModbusAsciiClientsCount = ( int16_t )tcpModbusAsciiServer->currentClientsCount();
	else
		status.tcpModbusAsciiClientsCount = -1;
	if( tcpModbusIpServer )
		status.tcpModbusIpClientsCount = ( int16_t )tcpModbusIpServer->currentClientsCount();
	else
		status.tcpModbusIpClientsCount = -1;
	status.sharedComPortClientsCount = sharedComPortNetworkClientsCount;
	mLinkServer->sendPacket( MarviePackets::Type::ServiceStatisticsType, ( uint8_t* )&status, sizeof( status ) );
}

void MarvieDevice::sendComPortSharingStatus()
{
	MarviePackets::ComPortSharingStatus status;
	status.sharedComPortIndex = -1;
	chSysLock();
	if( sharedComPortNetworkClientsCount != -1 )
		status.sharedComPortIndex = sharedComPortIndex;
	chSysUnlock();
	mLinkServer->sendPacket( MarviePackets::Type::ComPortSharingStatusType, ( uint8_t* )&status, sizeof( status ) );
}

void MarvieDevice::sendAnalogInputsData()
{
	struct Block
	{
		uint16_t blockId;
		uint16_t blockSize;
		float data[MarviePlatform::analogInputsCount];
	}* block = ( Block* )mLinkBuffer;
	block->blockId = 0;
	block->blockSize = MarviePlatform::analogInputsCount;
	for( uint32_t i = 0; i < MarviePlatform::analogInputsCount; ++i )
		block->data[i] = analogInput[i];
	mLinkServer->sendPacket( MarviePackets::Type::AnalogInputsDataType, mLinkBuffer, sizeof( Block ) );
}

void MarvieDevice::sendDigitInputsData()
{
	struct Block
	{
		uint16_t blockId;
		uint16_t blockSize;
		uint32_t data;
	} block;
	block.blockId = 0;
	block.blockSize = MarviePlatform::digitInputsCount;
	block.data = digitInputs;
	mLinkServer->sendPacket( MarviePackets::Type::DigitInputsDataType, ( uint8_t* )&block, sizeof( block ) );
}

uint32_t MarvieDevice::readConfigXmlFile( char** pXmlData )
{
	*pXmlData = nullptr;
	FIL* file = new FIL;
	configXmlFileMutex.lock();
	if( f_open( file, "/config.xml", FA_READ ) == FR_OK )
	{
		uint32_t size = ( uint32_t )f_size( file );
		char* xmlData = new char[size];
		UINT br;
		if( f_read( file, xmlData, size, &br ) == FR_OK && br == size )
		{
			f_close( file );
			configXmlFileMutex.unlock();
			delete file;

			*pXmlData = xmlData;
			return size;
		}
		delete xmlData;
		f_close( file );
	}
	configXmlFileMutex.unlock();
	delete file;

	return 0;
}

SHA1::Digest MarvieDevice::calcConfigXmlHash()
{
	SHA1::Digest hash;
	FIL* file = new FIL;
	configXmlFileMutex.lock();
	if( f_open( file, "/config.xml", FA_READ ) == FR_OK )
	{
		sha.reset();
		auto s = f_size( file );
		while( s )
		{
			auto t = s;
			if( t > 1024 )
				t = 1024;
			UINT br;
			if( f_read( file, buffer, t, &br ) != FR_OK || br != t )
				goto Skip;
			sha.addBytes( ( const char* )buffer, t );
			s -= t;
		}
		hash = sha.result();
		f_close( file );
	}
Skip:
	configXmlFileMutex.unlock();
	delete file;

	return hash;
}

FRESULT MarvieDevice::clearDir( const char* path )
{
	static FILINFO fno;
	FRESULT res;
	DIR dir;

	res = f_opendir( &dir, path );
	if( res == FR_OK )
	{
		while( ( ( res = f_readdir( &dir, &fno ) ) == FR_OK ) && fno.fname[0] )
		{
			if( FF_FS_RPATH && fno.fname[0] == '.' )
				continue;
			char* fn = fno.fname;

			size_t len = strlen( path );
			size_t fnLen = strlen( fn );
			char* nextPath;
			if( path[len - 1] == '/' )
			{
				nextPath = new char[len + fnLen + 1];
				strcpy( nextPath, path );
				strcpy( nextPath + len, fn );
			}
			else
			{
				nextPath = new char[len + 1 + fnLen + 1];
				strcpy( nextPath, path );
				*( nextPath + len ) = '/';
				strcpy( nextPath + len + 1, fn );
			}

			if( fno.fattrib & AM_DIR )
			{
				res = clearDir( nextPath );
				if( res != FR_OK )
				{
					delete nextPath;
					break;
				}
			}

			res = f_unlink( nextPath );
			delete nextPath;
			if( res != FR_OK )
				break;
		}
	}
	return res;
}

void MarvieDevice::removeOpenDatFiles()
{
	datFilesMutex.lock();
	for( int i = 0; i < 3; ++i )
	{
		if( datFiles[i] )
		{
			char path[] = "/Temp/0.dat";
			path[6] = '0' + i;
			f_close( datFiles[i] );
			f_unlink( path );
			delete datFiles[i];
			datFiles[i] = nullptr;
		}
	}
	datFilesMutex.unlock();
}

ModbusPotato::modbus_exception_code::modbus_exception_code MarvieDevice::read_input_registers( uint16_t address, uint16_t count, uint16_t* result )
{
	//constexpr uint16_t marvieRegBegin = 0;
	constexpr uint16_t digitInputRegBegin = 200;
	constexpr uint16_t analogInputRegBegin = 216;
	constexpr uint16_t sensorRegBegin = 600;

	// Temp
	for( ; address < digitInputRegBegin && count; address++, count-- )
		*result++ = 0;

	if( address >= digitInputRegBegin )
	{
		for( ; address < analogInputRegBegin && count; address++, count-- )
		{
			if( address == digitInputRegBegin )
				*result++ = ( ( uint16_t* )&digitInputs )[0];
			else if( address == digitInputRegBegin + 1 )
				*result++ = ( ( uint16_t* )&digitInputs )[1];
			else
				*result++ = 0;
		}
	}

	if( address >= analogInputRegBegin )
	{
		if( address < sensorRegBegin && ( address & 0x01 ) ) // not aligned
			return ModbusPotato::modbus_exception_code::illegal_data_address;
		for( ; address < sensorRegBegin && count >= 2; address += 2, count -= 2 )
		{
			uint16_t addr = ( address - analogInputRegBegin ) / 2;
			if( addr < MarviePlatform::analogInputsCount )
			{
				float data = analogInput[addr];
				*result++ = ( ( uint16_t* )&data )[0];
				*result++ = ( ( uint16_t* )&data )[1];
			}
			else
				*result++ = 0, *result++ = 0;
		}
	}

	if( address >= sensorRegBegin )
	{
		address -= sensorRegBegin;
		modbusRegistersMutex.lock();
		if( count > modbusRegistersCount || address >= modbusRegistersCount || ( size_t )( address + count ) > modbusRegistersCount )
		{
			modbusRegistersMutex.unlock();
			return ModbusPotato::modbus_exception_code::illegal_data_address;
		}

		// copy the values
		for( ; count; address++, count-- )
			*result++ = modbusRegisters[address];

		modbusRegistersMutex.unlock();
	}

	return ModbusPotato::modbus_exception_code::ok;
}

void MarvieDevice::mainTaskThreadTimersCallback( eventmask_t emask )
{
	chSysLockFromISR();
	instance()->mainThread->signalEventsI( emask );
	chSysUnlockFromISR();
}

void MarvieDevice::miskTaskThreadTimersCallback( eventmask_t emask )
{
	chSysLockFromISR();
	instance()->miskTasksThread->signalEventsI( emask );
	chSysUnlockFromISR();
}

void MarvieDevice::mLinkServerHandlerThreadTimersCallback( eventmask_t emask )
{
	chSysLockFromISR();
	instance()->mLinkServerHandlerThread->signalEventsI( emask );
	chSysUnlockFromISR();
}


void MarvieDevice::terminalOutputTimerCallback()
{
	chSysLockFromISR();
	terminalOutputThread->signalEventsI( TerminalOutputThreadEvent::TerminalOutputEvent );
	chSysUnlockFromISR();
}

void MarvieDevice::adInputsReadThreadTimersCallback( eventmask_t emask )
{
	chSysLockFromISR();
	instance()->adInputsReadThread->signalEventsI( emask );
	chSysUnlockFromISR();
}

void MarvieDevice::adcCallback( ADCDriver *adcp, adcsample_t *buffer, size_t n )
{
	if( n == 8 )
	{
		chSysLockFromISR();
		chThdResumeI( &MarvieDevice::instance()->adInputsReadThreadRef, MSG_OK );
		chSysUnlockFromISR();
	}
}

void MarvieDevice::adcErrorCallback( ADCDriver *adcp, adcerror_t err )
{
	chThdResumeI( &MarvieDevice::instance()->adInputsReadThreadRef, MSG_RESET );
}

void MarvieDevice::powerDownExtiCallback( void* arg )
{
	auto& failureDesc = MarvieBackup::instance()->failureDesc;
	chSysLockFromISR();
	failureDesc.pwrDown.dateTime = DateTimeService::currentDateTime();
	failureDesc.pwrDown.detected = true;
	MarvieDevice::instance()->mainThread->signalEventsI( MarvieDevice::MainThreadEvent::PowerDownDetected );
	chSysUnlockFromISR();
}

void MarvieDevice::faultHandler()
{
	auto& failureDesc = MarvieBackup::instance()->failureDesc;
	failureDesc.type = ( MarvieBackup::FailureDesc::Type )( __get_IPSR() + 1 );
	failureDesc.u.failure.flags = SCB->CFSR;
	port_extctx* ctx = ( port_extctx* )__get_PSP();
	failureDesc.u.failure.pc = ( uint32_t )ctx->pc;
	failureDesc.u.failure.lr = ( uint32_t )ctx->lr_thd;
	if( failureDesc.type == MarvieBackup::FailureDesc::Type::HardFault || failureDesc.type == MarvieBackup::FailureDesc::Type::BusFault )
		failureDesc.u.failure.busAddress = SCB->BFAR;
	else if( failureDesc.type == MarvieBackup::FailureDesc::Type::MemManage )
		failureDesc.u.failure.busAddress = SCB->MMFAR;
	else
		failureDesc.u.failure.busAddress = 0;
	failureDesc.threadAddress = ( uint32_t )currp;
	strncpy( failureDesc.threadName, "none", 5 ); // fix this!
	assert( false );
	NVIC_SystemReset();
}

void MarvieDevice::systemHaltHook( const char* reason )
{
	auto& failureDesc = MarvieBackup::instance()->failureDesc;
	failureDesc.type = MarvieBackup::FailureDesc::Type::SystemHalt;
	size_t len = strlen( reason );
	if( len > sizeof( failureDesc.u.msg ) - 1 )
		len = sizeof( failureDesc.u.msg ) - 1;
	strncpy( failureDesc.u.msg, reason, len );
	failureDesc.u.msg[len] = 0;
	failureDesc.threadAddress = ( uint32_t )currp;
	strncpy( failureDesc.threadName, "none", 5 ); // fix this!
	assert( false );
	NVIC_SystemReset();
}

int MarvieDevice::rtc( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] )
{
	constexpr char invalidArgsError[] = "invalid arguments";
	if( argc != 0 && argc != 2 )
	{
		terminal->stdErrWrite( ( uint8_t* )invalidArgsError, sizeof( invalidArgsError ) - 1 );
		return -1;
	}
	if( argc == 2 )
	{
		if( strcmp( argv[0], "-c" ) == 0 )
		{
			int calibValue = atoi( argv[1] );
			if( calibValue > 511 )
				calibValue = 511;
			else if( calibValue < -511 )
				calibValue = -511;

			uint32_t calr = 0;
			if( calibValue > 0 )
			{
				calr |= RTC_CALR_CALP_Msk;
				calr |= ( uint32_t )( calibValue ) << RTC_CALR_CALM_Pos;
			}
			else if( calibValue < 0 )
				calr |= ( uint32_t )( -calibValue ) << RTC_CALR_CALM_Pos;

			while( RTCD1.rtc->ISR & RTC_ISR_RECALPF )
				;
			RTCD1.rtc->CALR = calr;

			return 0;
		}
		else
		{
			terminal->stdErrWrite( ( uint8_t* )invalidArgsError, sizeof( invalidArgsError ) - 1 );
			return -1;
		}
	}

	char s[30];
	DateTimeService::currentDateTime().printDateTime( s )[0] = 0;
	std::string str( "Current date: " );
	str.append( s );
	str.append( "\nLast SNTP update: " );
	MarvieBackup::instance()->acquire();
	auto lastSync = MarvieBackup::instance()->settings.dateTime.lastSntpSync;
	MarvieBackup::instance()->release();
	if( lastSync == -1 )
		str.append( "unknown" );
	else
	{
		DateTime::fromMsecsSinceEpoch( lastSync ).printDateTime( s )[0] = 0;
		str.append( s );
	}
	str.append( "\nCalib value: " );
	uint32_t calr = RTCD1.rtc->CALR;
	int32_t calrm = ( int32_t )( ( calr & RTC_CALR_CALM ) >> RTC_CALR_CALM_Pos );
	if( ( calr & RTC_CALR_CALP_Msk ) == 0 )
		calrm = -calrm;
	str.append( std::to_string( calrm ) );
	terminal->stdOutWrite( ( uint8_t* )str.c_str(), str.size() );

	return 0;
}

int MarvieDevice::lgbt( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] )
{
	const char text[] = "LGBT LGBT LGBT LGBT LGBT LGBT LGBT LGBT\n";
	terminal->setBackgroundColor( 225, 23, 23 );
	terminal->setTextColor( 225, 23, 23 );
	terminal->stdOutWrite( ( uint8_t* )text, sizeof( text ) - 1 );

	terminal->setBackgroundColor( 252, 144, 25 );
	terminal->setTextColor( 252, 144, 25 );
	terminal->stdOutWrite( ( uint8_t* )text, sizeof( text ) - 1 );

	terminal->setBackgroundColor( 252, 242, 25 );
	terminal->setTextColor( 252, 242, 25 );
	terminal->stdOutWrite( ( uint8_t* )text, sizeof( text ) - 1 );

	terminal->setBackgroundColor( 7, 130, 38 );
	terminal->setTextColor( 7, 130, 38 );
	terminal->stdOutWrite( ( uint8_t* )text, sizeof( text ) - 1 );

	terminal->setBackgroundColor( 48, 50, 254 );
	terminal->setTextColor( 48, 50, 254 );
	terminal->stdOutWrite( ( uint8_t* )text, sizeof( text ) - 1 );

	terminal->setBackgroundColor( 117, 0, 135 );
	terminal->setTextColor( 117, 0, 135 );
	terminal->stdOutWrite( ( uint8_t* )text, sizeof( text ) - 1 );

	return 0;
}

int MarvieDevice::qAsciiArtFunction( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] )
{
	terminal->stdOutWrite( ( const uint8_t* )quentinAsciiArt, sizeof( quentinAsciiArt ) );
	return 0;
}

extern "C"
{
	void systemHaltHook( const char *reason )
	{
		MarvieDevice::systemHaltHook( reason );
	}

	void idleLoopHook()
	{
		wdgReset( &WDGD1 );
	}

	void sntpSetSystemTimeCallback( int32_t sec, uint32_t us )
	{
		MarvieBackup* backup = MarvieBackup::instance();
		sec += backup->settings.dateTime.timeZone * 3600;
		DateTime syncDateTime( Date::fromDaysSinceEpoch( sec / 86400 + 719468 ), Time::fromMsecsSinceStartOfDay( ( sec % 86400 ) * 1000 + us / 1000 ) );
		DateTime currentDateTime = DateTimeService::currentDateTime();
		backup->acquire();
		DateTimeService::setDateTime( syncDateTime );

		int64_t syncMsecs = syncDateTime.msecsSinceEpoch();
		int64_t currMsecs = currentDateTime.msecsSinceEpoch();
		int64_t d = syncMsecs - backup->settings.dateTime.lastSntpSync;

		if( backup->settings.dateTime.lastSntpSync != -1 && d >= ( SNTP_UPDATE_DELAY - 600000 ) )
		{
			int64_t calib = ( syncMsecs - currMsecs ) * STM32_LSECLK * 32 / d;
			uint32_t calr = RTCD1.rtc->CALR;
			int32_t calrm = ( int32_t )( ( calr & RTC_CALR_CALM ) >> RTC_CALR_CALM_Pos );
			if( ( calr & RTC_CALR_CALP_Msk ) == 0 )
				calrm = -calrm;
			calrm += ( int32_t )calib;
			if( calrm > 511 )
				calrm = 511;
			else if( calrm < -511 )
				calrm = -511;

			calr = 0;
			if( calrm > 0 )
			{
				calr |= RTC_CALR_CALP_Msk;
				calr |= ( uint32_t )( calrm ) << RTC_CALR_CALM_Pos;
			}
			else if( calrm < 0 )
				calr |= ( uint32_t )( -calrm ) << RTC_CALR_CALM_Pos;

			while( RTCD1.rtc->ISR & RTC_ISR_RECALPF )
				;
			RTCD1.rtc->CALR = calr;
		}
		backup->settings.dateTime.lastSntpSync = syncMsecs;
		backup->settings.setValid();
		backup->release();
	}

	void NMI_Handler()
	{
		MarvieDevice::faultHandler();
	}

	void HardFault_Handler()
	{
		MarvieDevice::faultHandler();
	}

	void BusFault_Handler()
	{
		MarvieDevice::faultHandler();
	}

	void MemManage_Handler()
	{
		MarvieDevice::faultHandler();
	}

	void UsageFault_Handler()
	{
		MarvieDevice::faultHandler();
	}
}