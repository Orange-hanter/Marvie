#include "MarvieDevice.h"
#include "Core/CCMemoryAllocator.h"
#include "Core/DateTimeService.h"
#include "Core/CCMemoryHeap.h"
#include "lwipthread.h"
#include "Network/UdpSocket.h"
#include "Network/TcpServer.h"

#include "Tests/UdpStressTestServer/UdpStressTestServer.h"

using namespace MarvieXmlConfigParsers;

MarvieDevice* MarvieDevice::_inst = nullptr;

MarvieDevice::MarvieDevice() : backupRegs( 20 ), configXmlDataSendingSemaphore( false )
{
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

	// SimGsm PWR KEY // TEMP!
	palSetPadMode( GPIOD, 1, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST );
	palSetPad( GPIOD, 1 );

	// PA8 - CD
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
	sdCardStatus = SdCardStatus::NotInserted;
	sdCardInserted = false;
	sdCardStatusEventPending = false;
	f_mount( &fatFs, "/", 0 );
	fsError = FR_NOT_READY;

	crcStart( &CRCD1, nullptr );

	Backup& backup = ( Backup& )backupRegs.value( 0 );
	if( !backupRegs.isValid() )
	{
		backup.flags.ethernetDhcp = true;
		backup.eth.ip = IpAddress( 192, 168, 2, 10 );
		backup.eth.netmask = 0xFFFFFF00;
		backup.eth.gateway = IpAddress( 192, 168, 2, 1 );
		backupRegs.setValid();
	}

	deviceState = DeviceState::IncorrectConfiguration;
	configError = ConfigError::NoError;
	sensorsConfigError = SensorsConfigError::NoError;
	errorSensorName = nullptr;
	errorSensorId = 0;

	configNum = 0;
	vPorts = nullptr;
	vPortBindings = nullptr;
	vPortsCount = 0;
	sensors = nullptr;
	sensorsCount = 0;
	brSensorReaders = nullptr;
	brSensorReaderListeners = nullptr;
	gsmModem = nullptr;
	rawModbusServers = nullptr;
	rawModbusServersCount = 0;
	tcpModbusRtuServer = nullptr;
	tcpModbusAsciiServer = nullptr;
	tcpModbusIpServer = nullptr;
	modbusRegisters = nullptr;
	modbusRegistersCount = 0;

	syncConfigNum = ( uint32_t )-1;
	datFiles[0] = datFiles[1] = datFiles[2] = nullptr;

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

	auto ethConf = EthernetThread::instance()->currentConfig();
	ethConf.addressMode = backup.flags.ethernetDhcp ? EthernetThread::AddressMode::Dhcp : EthernetThread::AddressMode::Static;
	ethConf.ipAddress = backup.eth.ip;
	ethConf.netmask = backup.eth.netmask;
	ethConf.gateway = backup.eth.gateway;
	EthernetThread::instance()->setConfig( ethConf );
	EthernetThread::instance()->startThread( EthernetThreadPrio );

	mLinkServer = new MLinkServer;
	mLinkServer->setComplexDataReceiveCallback( this );
	mLinkServer->setIODevice( comPorts[MarviePlatform::mLinkComPort] );

	/*UdpSocket* socket = new UdpSocket;
	socket->bind( 16021 );
	socket->connect( IpAddress( 192, 168, 2, 1 ), 16032 );
	mLinkServer->setIODevice( socket );*/
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
	mainThread               = Concurrent::run( [this]() { mainThreadMain(); },               2048, NORMALPRIO );
	miskTasksThread          = Concurrent::run( [this]() { miskTasksThreadMain(); },          1024, NORMALPRIO );
	adInputsReadThread       = Concurrent::run( [this]() { adInputsReadThreadMain(); },       512,  NORMALPRIO + 2 );
	mLinkServerHandlerThread = Concurrent::run( [this]() { mLinkServerHandlerThreadMain(); }, 1024, NORMALPRIO );
	uiThread                 = Concurrent::run( [this]() { uiThreadMain(); },                 1024, NORMALPRIO );

	//mLinkServer->startListening( NORMALPRIO );
		
	Concurrent::run( []()
	{
		UdpStressTestServer* server = new UdpStressTestServer( 1112 );
		server->exec();
	}, 2048, NORMALPRIO );

	Concurrent::run( []()
	{
		UdpStressTestServer* server = new UdpStressTestServer( 1114 );
		server->exec();
	}, 2048, NORMALPRIO );

	TcpServer* server = new TcpServer;
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
	}

	thread_reference_t ref = nullptr;
	chSysLock();
	chThdSuspendS( &ref );
}

void MarvieDevice::mainThreadMain()
{
	while( true )
	{
		eventmask_t em = chEvtWaitAny( ALL_EVENTS );
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
					if( fsError == FR_OK && fatFs.fs_type == FS_FAT32 )
					{
						fsError = clearDir( "/Temp" );
						if( fsError == FR_NO_PATH )
							fsError = f_mkdir( "/Temp" );
						if( fsError == FR_OK )
						{
							sdCardStatus = SdCardStatus::Working;
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
				if( brSensorReaders[i] )
					brSensorReaders[i]->startReading( NORMALPRIO );
			}
		}
		if( em & MainThreadEvent::StopSensorReaders )
		{
			for( uint i = 0; i < vPortsCount; ++i )
			{
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
							sensors[sensor->userData()].updated = true;
							copySensorDataToModbusRegisters( sensor );
							sensor = reader->nextUpdatedSensor();
						}
					}
					else if( vPortBindings[i] == VPortBinding::Rs232 || vPortBindings[i] == VPortBinding::NetworkSocket )
					{
						AbstractBRSensor* sensor = static_cast< SingleBRSensorReader* >( brSensorReaders[i] )->nextSensor();
						sensors[sensor->userData()].updated = true;
						copySensorDataToModbusRegisters( sensor );
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
			srSensorsUpdateTimer.set( TIME_MS2I( SrSensorInterval ), mainTaskThreadTimersCallback, ( void* )MainThreadEvent::SrSensorsTimerEvent );
			for( uint i = 0; i < sensorsCount; ++i )
			{
				if( sensors[i].sensor->type() == AbstractSensor::Type::SR )
					sensors[i].updated = true;
			}
			mLinkServerHandlerThread->signalEvents( MLinkThreadEvent::SensorUpdateEvent );
		}
		if( em & MainThreadEvent::RestartRequestEvent )
		{
			ejectSdCard();
			__NVIC_SystemReset();
		}
		if( em & MainThreadEvent::EjectSdCardRequest )
			ejectSdCard();
		if( em & MainThreadEvent::FormatSdCardRequest )
			formatSdCard();
	}
}

void MarvieDevice::miskTasksThreadMain()
{
	virtual_timer_t sdTestTimer, memoryTestTimer;
	chVTObjectInit( &sdTestTimer );
	chVTObjectInit( &memoryTestTimer );
	chVTSet( &sdTestTimer, TIME_MS2I( SdTestInterval ), miskTaskThreadTimersCallback, ( void* )MiskTaskThreadEvent::SdCardTestEvent );
	chVTSet( &memoryTestTimer, TIME_MS2I( MemoryTestInterval ), miskTaskThreadTimersCallback, ( void* )MiskTaskThreadEvent::MemoryTestEvent );
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
			chVTSet( &sdTestTimer, TIME_MS2I( SdTestInterval ), miskTaskThreadTimersCallback, ( void* )MiskTaskThreadEvent::SdCardTestEvent );
		}
		if( em & MiskTaskThreadEvent::MemoryTestEvent )
		{
			memoryLoad.freeGRam = chCoreGetStatusX();
			memoryLoad.freeCcRam = CCMemoryAllocator::status();
			memoryLoad.gRamHeapFragments = chHeapStatus( nullptr, ( size_t* )&memoryLoad.gRamHeapSize, ( size_t* )&memoryLoad.gRamHeapLargestFragmentSize );
			memoryLoad.ccRamHeapFragments = CCMemoryHeap::status( ( size_t* )&memoryLoad.ccRamHeapSize, ( size_t* )&memoryLoad.ccRamHeapLargestFragmentSize );
			if( sdCardStatus == SdCardStatus::Working )
			{
				memoryLoad.sdCardCapacity = ( fatFs.n_fatent - 2 ) * fatFs.csize * 512;
				memoryLoad.sdCardFreeSpace = fatFs.free_clst * fatFs.csize * 512;
			}
			else
			{
				memoryLoad.sdCardCapacity = 0;
				memoryLoad.sdCardFreeSpace = 0;
			}
			mLinkServerHandlerThread->signalEvents( MLinkThreadEvent::MemoryLoadEvent );
			chVTSet( &memoryTestTimer, TIME_MS2I( MemoryTestInterval ), miskTaskThreadTimersCallback, ( void* )MiskTaskThreadEvent::MemoryTestEvent );
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
			//avg /= 8;
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
		if( TIME_MS2I( 50 ) > dt )
			chThdSleepMilliseconds( TIME_MS2I( 50 ) - dt );
	}
}

void MarvieDevice::mLinkServerHandlerThreadMain()
{
	EvtListener mLinkListener, cpuUsageMonitorListener, ethThreadListener;
	mLinkServer->eventSource()->registerMask( &mLinkListener, MLinkThreadEvent::MLinkEvent );
	CpuUsageMonitor::instance()->eventSource()->registerMask( &cpuUsageMonitorListener, MLinkThreadEvent::CpuUsageMonitorEvent );
	EthernetThread::instance()->eventSource()->registerMask( &ethThreadListener, MLinkThreadEvent::EthernetEvent );

	virtual_timer_t timer;
	chVTObjectInit( &timer );

	syncConfigNum = ( uint32_t )-1;
	auto sensorDataChannel = mLinkServer->createComplexDataChannel();

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
					chVTSet( &timer, TIME_MS2I( StatusInterval ), mLinkServerHandlerThreadTimersCallback, ( void* )MLinkThreadEvent::StatusUpdateEvent );
				}
				else
					chVTReset( &timer );
			}
			if( flags & ( eventflags_t )MLinkServer::Event::NewPacketAvailable )
			{
				uint8_t type;
				uint32_t size = mLinkServer->readPacket( &type, mLinkBuffer, sizeof( mLinkBuffer ) );
				mLinkProcessNewPacket( type, mLinkBuffer, size );
			}
		}
		if( mLinkServer->state() != MLinkServer::State::Connected )
			continue;
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
					if( !sensors[i].updated )
						continue;
					sensors[i].updated = false;
					sendSensorDataM( i, sensorDataChannel );
				}
			}
			configMutex.unlock();
		}
		if( em & MLinkThreadEvent::StatusUpdateEvent )
		{
			chVTSet( &timer, TIME_MS2I( StatusInterval ), mLinkServerHandlerThreadTimersCallback, ( void* )MLinkThreadEvent::StatusUpdateEvent );
			sendDeviceStatus();
			sendAnalogInputsData();
			sendDigitInputsData();

			configMutex.lock();
			sendServiceStatisticsM();
			if( syncConfigNum == configNum )
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
			MarviePackets::MemoryLoad load = memoryLoad;
			mLinkServer->sendPacket( MarviePackets::Type::MemoryLoadType, ( uint8_t* )&load, sizeof( load ) );
		}
		if( em & MLinkThreadEvent::EthernetEvent )
			sendEthernetStatus();
		if( em & MLinkThreadEvent::GsmModemEvent )
		{
			configMutex.lock();
			sendGsmModemStatusM();
			configMutex.unlock();
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

void MarvieDevice::ejectSdCard()
{
	if( sdCardStatus != SdCardStatus::NotInserted )
	{
		sdCardStatus = SdCardStatus::NotInserted;
		removeOpenDatFiles();
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
		if( f_mkfs( "/", FM_FAT32, 1024, buffer, 1024 ) == FR_OK && f_mkdir( "/Temp" ) == FR_OK )
			sdCardStatus = SdCardStatus::Working;
		else
			sdCardStatus = SdCardStatus::BadFileSystem;
		configXmlHash = SHA1::Digest();
	}
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
			mLinkServerHandlerThread->signalEvents( ( eventmask_t )MLinkThreadEvent::ConfigChangedEvent );
	}
	else
	{
		configXmlHash = SHA1::Digest();
		configError = ConfigError::NoConfigFile;
		deviceState = DeviceState::IncorrectConfiguration;
	}
}

void MarvieDevice::configShutdown()
{
	for( uint i = 0; i < vPortsCount; ++i )
	{
		if( brSensorReaders[i] )
			brSensorReaders[i]->stopReading();
	}
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
	for( uint i = 0; i < rawModbusServersCount; ++i )
		rawModbusServers[i].waitForStateChange();
	if( tcpModbusRtuServer )
		tcpModbusRtuServer->waitForStateChange();
	if( tcpModbusAsciiServer )
		tcpModbusAsciiServer->waitForStateChange();
	if( tcpModbusIpServer )
		tcpModbusIpServer->waitForStateChange();

	removeConfigRelatedObject();
	srSensorsUpdateTimer.reset();
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
	SensorReadingConf sensorReadingConf;
	if( !parseSensorReadingConfig( rootNode->FirstChildElement( "sensorReadingConfig" ), &sensorReadingConf ) )
	{
		deviceState = DeviceState::IncorrectConfiguration;
		configError = ConfigError::SensorReadingConfigError;

		delete doc;
		return;
	}

	auto ethThd = EthernetThread::instance();
	auto ethThdConf = ethThd->currentConfig();
	if( ( networkConf.ethConf.dhcpEnable == true && ethThdConf.addressMode != EthernetThread::AddressMode::Dhcp ) ||
		( networkConf.ethConf.dhcpEnable == false && ethThdConf.addressMode == EthernetThread::AddressMode::Dhcp ) ||
		networkConf.ethConf.ip != ethThdConf.ipAddress ||
		networkConf.ethConf.netmask != ethThdConf.netmask ||
		networkConf.ethConf.gateway != ethThdConf.gateway )
	{
		Backup& backup = ( Backup& )backupRegs.value( 0 );
		backup.flags.ethernetDhcp = networkConf.ethConf.dhcpEnable;
		backup.eth.ip = networkConf.ethConf.ip;
		backup.eth.netmask = networkConf.ethConf.netmask;
		backup.eth.gateway = networkConf.ethConf.gateway;
		backupRegs.setValid();

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
		{
			gsmModem = new SimGsmPppModem( MarviePlatform::gsmModemEnableIoPort, false, MarviePlatform::gsmModemEnableLevel );
			gsmModem->setAsDefault();
			auto gsmUsart = comUsarts[MarviePlatform::comPortIoLines[MarviePlatform::gsmModemComPort].comUsartIndex].usart;
			if( MarviePlatform::comPortTypes[MarviePlatform::gsmModemComPort] == ComPortType::Rs485 )
				static_cast< AbstractRs485* >( comPorts[MarviePlatform::gsmModemComPort] )->disable();
			gsmUsart->setBaudRate( 230400 );
			gsmUsart->setDataFormat( UsartBasedDevice::B8N );
			gsmUsart->setStopBits( UsartBasedDevice::S1 );
			gsmModem->setUsart( gsmUsart );
			gsmModem->eventSource()->registerMask( &gsmModemListener, MLinkThreadEvent::GsmModemEvent );
			/* Hack! */
			gsmModemListener.ev_listener.listener = mLinkServerHandlerThread->thread_ref;
			/* You didn't see anything */
		}
		if( gsmModem->pinCode() != gsmConf->pinCode || strcmp( gsmModem->apn(), gsmConf->apn ) != 0 )
		{
			gsmModem->stopModem();
			gsmModem->waitForStateChange();
			gsmModem->setPinCode( gsmConf->pinCode );
			strcpy( apn, gsmConf->apn );
			gsmModem->setApn( apn );
			gsmModem->startModem();
		}
	}
	else if( gsmModem )
	{
		gsmModem->stopModem();
		gsmModem->waitForStateChange();
		gsmModem->eventSource()->unregister( &gsmModemListener );
		delete gsmModem;
		gsmModem = nullptr;
	}

	vPortsCount = 0;
	rawModbusServersCount = 0;
	enum class ModbusComPortType : uint8_t { None, Rtu, Ascii } modbusComPortTypes[MarviePlatform::comPortsCount];
	for( uint i = 0; i < MarviePlatform::comPortsCount; ++i )
	{
		auto assignment = comPortsConf[i]->assignment();
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

	uint16_t modbusRtuServerPort = networkConf.modbusRtuServerPort;
	uint16_t modbusAsciiServerPort = networkConf.modbusAsciiServerPort;
	uint16_t modbusIpServerPort = networkConf.modbusTcpServerPort;
	volatile bool modbusEnabled = rawModbusServersCount != 0 || networkConf.modbusTcpServerPort != 0 ||
		networkConf.modbusAsciiServerPort != 0 || networkConf.modbusRtuServerPort != 0;
	uint32_t modbusOffset = 0;

	vPorts = new IODevice*[vPortsCount];
	vPortBindings = new VPortBinding[vPortsCount];
	brSensorReaders = new BRSensorReader*[vPortsCount];
	brSensorReaderListeners = new EvtListener[vPortsCount];
	for( uint i = 0; i < vPortsCount; ++i )
	{
		vPorts[i] = nullptr;
		vPortBindings[i] = VPortBinding::Rs232;
		brSensorReaders[i] = nullptr;
	}
	uint32_t n = 0;
	for( uint i = 0; i < MarviePlatform::comPortsCount; ++i )
	{
		if( comPortsConf[i]->assignment() == ComPortAssignment::VPort )
		{
			vPorts[n] = comPorts[i];
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
	errorSensorName = nullptr;
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
				sensors[i].updated = false;
			}
			sensorsCount = 0;
		}

		sensorNode = sensorsNode->FirstChildElement();
		while( sensorNode )
		{
			errorSensorName = sensorNode->Name();
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

				if( vPortBindings[vPortId] != VPortBinding::Rs485 && brSensorReaders[vPortId] ) // FIX?
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
					brSensorReaders[vPortId] = reader;
				}
				else if( vPortBindings[vPortId] == VPortBinding::Rs485 )
				{
					MultipleBRSensorsReader* reader;
					if( brSensorReaders[vPortId] )
						reader = static_cast< MultipleBRSensorsReader* >( brSensorReaders[vPortId] );
					else
					{
						brSensorReaders[vPortId] = reader = new MultipleBRSensorsReader;
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
			else
			{
				srSensorsEnabled = true;

				uint32_t blockId;
				XMLElement* c0 = sensorNode->FirstChildElement( "blockID" );
				if( !c0 || ( blockId = c0->UnsignedText( 8 ) ) >= 8 )
				{
					sensorsConfigError = SensorsConfigError::IncorrectSettings;
					break;
				}

				AbstractSensor* sensor = desc->allocator();
				sensor->setUserData( sensorsCount );
				sensors[sensorsCount].sensor = sensor;
				sensors[sensorsCount].vPortId = 0xFFFF;
				sensors[sensorsCount++].modbusStartRegNum = modbusOffset / 2;
				modbusOffset += sensor->sensorDataSize() + sizeof( DateTime );
				static_cast< AbstractSRSensor* >( sensor )->setInputSignalProvider( this );
				if( !desc->tuner( sensor, sensorNode, 0 ) )
				{
					delete sensor;
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
		if( modbusEnabled )
		{
			modbusRegisters = new uint16_t[modbusOffset / 2];
			memset( modbusRegisters, 0, modbusOffset );
			modbusRegistersCount = modbusOffset / 2;

			rawModbusServers = new RawModbusServer[rawModbusServersCount];
			for( uint32_t iPort = 0, iServer = 0; iPort < MarviePlatform::comPortsCount; ++iPort )
			{
				if( modbusComPortTypes[iPort] != ModbusComPortType::None )
				{
					rawModbusServers[iServer].setFrameType( modbusComPortTypes[iPort] == ModbusComPortType::Rtu ? ModbusDevice::FrameType::Rtu : ModbusDevice::FrameType::Ascii );
					rawModbusServers[iServer].setSlaveHandler( this );
					rawModbusServers[iServer].setIODevice( comPorts[iPort] );
					rawModbusServers[iServer++].startServer();
				}
			}

			if( modbusRtuServerPort )
			{
				tcpModbusRtuServer = new TcpModbusServer;
				tcpModbusRtuServer->setFrameType( ModbusDevice::FrameType::Rtu );
				tcpModbusRtuServer->setSlaveHandler( this );
				tcpModbusRtuServer->setPort( modbusRtuServerPort );
				tcpModbusRtuServer->startServer();
			}
			if( modbusAsciiServerPort )
			{
				tcpModbusAsciiServer = new TcpModbusServer;
				tcpModbusAsciiServer->setFrameType( ModbusDevice::FrameType::Ascii );
				tcpModbusAsciiServer->setSlaveHandler( this );
				tcpModbusAsciiServer->setPort( modbusAsciiServerPort );
				tcpModbusAsciiServer->startServer();
			}
			if( modbusIpServerPort )
			{
				tcpModbusIpServer = new TcpModbusServer;
				tcpModbusIpServer->setFrameType( ModbusDevice::FrameType::Ip );
				tcpModbusIpServer->setSlaveHandler( this );
				tcpModbusIpServer->setPort( modbusIpServerPort );
				tcpModbusIpServer->startServer();
			}
		}

		for( uint i = 0; i < vPortsCount; ++i )
		{
			brSensorReaderListeners[i].getAndClearFlags();
			if( brSensorReaders[i] )
			{
				brSensorReaders[i]->eventSource()->registerMask( brSensorReaderListeners + i, MainThreadEvent::BrSensorReaderEvent );
				brSensorReaders[i]->startReading( NORMALPRIO );
			}
		}

		if( srSensorsEnabled )
			srSensorsUpdateTimer.set( TIME_MS2I( SrSensorInterval ), mainTaskThreadTimersCallback, ( void* )MainThreadEvent::SrSensorsTimerEvent );

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
	for( uint i = 0; i < vPortsCount; ++i )
		delete brSensorReaders[i];
	for( uint i = 0; i < sensorsCount; ++i )
		delete sensors[i].sensor;

	delete vPorts;
	delete vPortBindings;
	delete sensors;
	delete brSensorReaders;
	delete brSensorReaderListeners;

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

	rawModbusServers = nullptr;
	rawModbusServersCount = 0;
	tcpModbusRtuServer = nullptr;
	tcpModbusAsciiServer = nullptr;
	tcpModbusIpServer = nullptr;
	modbusRegisters = nullptr;
	modbusRegistersCount = 0;
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
		return 0;
	}

	return 0;
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

uint32_t MarvieDevice::onOpennig( uint8_t id, const char* name, uint32_t size )
{
	datFilesMutex.lock();
	if( sdCardStatus != SdCardStatus::Working || id >= 3 || datFiles[id] )
	{
		datFilesMutex.unlock();
		return ( uint32_t )-1;
	}

	char path[] = "/Temp/0.dat";
	path[6] = '0' + id;
	datFiles[id] = new FIL;
	if( f_open( datFiles[id], path, FA_OPEN_ALWAYS | FA_WRITE | FA_READ ) != FR_OK )
	{
		delete datFiles[id];
		datFiles[id] = nullptr;

		datFilesMutex.unlock();
		return ( uint32_t )-1;
	}

	datFilesMutex.unlock();
	return 1;
}

bool MarvieDevice::newDataReceived( uint8_t id, const uint8_t* data, uint32_t size )
{
	datFilesMutex.lock();
	if( sdCardStatus != SdCardStatus::Working || id >= 3 || datFiles[id] == nullptr )
		goto Abort;

	uint bw;
	if( f_write( datFiles[id], data, size, &bw ) != FR_OK || bw != size )
		goto Abort;

	datFilesMutex.unlock();
	return true;

Abort:
	datFilesMutex.unlock();
	return false;
}

void MarvieDevice::onClosing( uint8_t id, bool canceled )
{
	datFilesMutex.lock();
	if( sdCardStatus != SdCardStatus::Working || id >= 3 || datFiles[id] == nullptr )
	{
		datFilesMutex.unlock();
		return;
	}

	if( canceled )
	{
		char path[] = "/Temp/0.dat";
		path[6] = '0' + id;
		f_close( datFiles[id] );
		f_unlink( path );
		delete datFiles[id];
		datFiles[id] = nullptr;
	}
	else
	{
		//f_sync( datFiles[id] );
		switch( id )
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

void MarvieDevice::mLinkProcessNewPacket( uint32_t type, uint8_t* data, uint32_t size )
{
	if( type == MarviePackets::Type::GetConfigXmlType )
	{
		if( configXmlDataSendingSemaphore.wait( TIME_IMMEDIATE ) == MSG_TIMEOUT )
			return;

		char* xmlData = nullptr;
		uint32_t xmlDataSize = readConfigXmlFile( &xmlData );
		if( xmlData )
		{
			auto channel = mLinkServer->createComplexDataChannel();
			if( channel->open( MarviePackets::ComplexChannel::XmlConfigChannel, "", xmlDataSize ) )
			{
				Concurrent::run( [this, channel, xmlData, xmlDataSize]()
				{
					channel->sendData( ( uint8_t* )xmlData, xmlDataSize );
					channel->close();
					delete xmlData;
					delete channel;
					configXmlDataSendingSemaphore.signal();
				}, 512, NORMALPRIO );
			}
		}
		else
		{
			configXmlDataSendingSemaphore.signal();
			mLinkServer->sendPacket( MarviePackets::Type::ConfigXmlMissingType, nullptr, 0 );
		}
	}
	else if( type == MarviePackets::Type::SetDateTimeType )
	{
		DateTime dateTime;
		strncpy( ( char* )&dateTime, ( const char* )data, sizeof( DateTime ) );
		DateTimeService::setDateTime( dateTime );
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
	else if( type == MarviePackets::Type::FormatSdCardType )
	{
		if( sdCardStatus != SdCardStatus::Formatting )
			mainThread->signalEvents( MainThreadEvent::FormatSdCardRequest );
	}
}

void MarvieDevice::mLinkSync( bool coldSync )
{
	mLinkServer->sendPacket( MarviePackets::Type::SyncStartType, nullptr, 0 );
	auto channel = mLinkServer->createComplexDataChannel();
	if( coldSync )
	{
		auto channel = mLinkServer->createComplexDataChannel();
		if( channel->open( MarviePackets::ComplexChannel::SupportedSensorsListChannel, "", 0 ) )
		{
			char* data = new char[1024];
			uint32_t n = 0;
			for( auto i = SensorService::beginSensorsList(); i != SensorService::endSensorsList(); ++i )
			{
				uint32_t len = strlen( ( *i ).name );
				if( 1024 - n < len )
					channel->sendData( ( uint8_t* )data, n ), n = 0;
				strncpy( data + n, ( *i ).name, len );
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

		sendFirmwareDesc();
	}

	syncConfigNum = configNum;

	// sending config.xml
	char* xmlData = nullptr;
	uint32_t xmlDataSize = readConfigXmlFile( &xmlData );
	if( xmlDataSize )
	{
		configXmlDataSendingSemaphore.wait( TIME_INFINITE );
		auto channel = mLinkServer->createComplexDataChannel();
		if( channel->open( MarviePackets::ComplexChannel::XmlConfigSyncChannel, "", xmlDataSize ) )
		{
			channel->sendData( ( uint8_t* )xmlData, xmlDataSize );
			channel->close();
		}
		delete channel;
		delete xmlData;
		configXmlDataSendingSemaphore.signal();
	}

	sendDeviceStatus();
	sendEthernetStatus();
	sendAnalogInputsData();
	sendDigitInputsData();

	configMutex.lock();
	sendGsmModemStatusM();
	sendServiceStatisticsM();
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
	strcpy( desc.coreVersion, MarviePlatform::coreVersion );
	strcpy( desc.targetName, MarviePlatform::platformType );
	mLinkServer->sendPacket( MarviePackets::Type::FirmwareDescType, ( uint8_t* )&desc, sizeof( desc ) );
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

void MarvieDevice::sendSensorDataM( uint32_t sensorId, MLinkServer::ComplexDataChannel* channel )
{
	auto sensorData = sensors[sensorId].sensor->sensorData();
	sensorData->lock();
	if( sensorData->isValid() )
	{
		if( channel->open( MarviePackets::ComplexChannel::SensorDataChannel, nullptr, 0 ) )
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
		report.dateTime = sensorData->time();
		mLinkServer->sendPacket( MarviePackets::Type::SensorErrorReportType, ( const uint8_t* )&report, sizeof( report ) );
	}
	sensorData->unlock();
}

void MarvieDevice::sendDeviceStatus()
{
	MarviePackets::DeviceStatus status;
	status.dateTime = DateTimeService::currentDateTime();
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
		status.state = MarviePackets::GsmStatus::State::Stopped;
		status.error = MarviePackets::GsmStatus::Error::NoError;
	}
	mLinkServer->sendPacket( MarviePackets::Type::GsmStatusType, ( uint8_t* )&status, sizeof( status ) );
}

void MarvieDevice::sendServiceStatisticsM()
{
	MarviePackets::ServiceStatistics status;
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
	mLinkServer->sendPacket( MarviePackets::Type::ServiceStatisticsType, ( uint8_t* )&status, sizeof( status ) );
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
	return ModbusPotato::modbus_exception_code::ok;
}

void MarvieDevice::mainTaskThreadTimersCallback( void* p )
{
	chSysLockFromISR();
	instance()->mainThread->signalEventsI( ( eventmask_t )p );
	chSysUnlockFromISR();
}

void MarvieDevice::miskTaskThreadTimersCallback( void* p )
{
	chSysLockFromISR();
	instance()->miskTasksThread->signalEventsI( ( eventmask_t )p );
	chSysUnlockFromISR();
}

void MarvieDevice::mLinkServerHandlerThreadTimersCallback( void* p )
{
	chSysLockFromISR();
	instance()->mLinkServerHandlerThread->signalEventsI( ( eventmask_t )p );
	chSysUnlockFromISR();
}

void MarvieDevice::adInputsReadThreadTimersCallback( void* p )
{
	chSysLockFromISR();
	instance()->adInputsReadThread->signalEventsI( ( eventmask_t )p );
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