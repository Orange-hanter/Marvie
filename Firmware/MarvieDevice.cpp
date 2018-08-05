#include "MarvieDevice.h"
#include "Core/CCMemoryAllocator.h"
#include "Core/DateTimeService.h"
#include "Core/CCMemoryHeap.h"
#include "lwipthread.h"
#include "Network/UdpSocket.h"
#include "Network/TcpServer.h"

#include "Tests/UdpStressTestServer/UdpStressTestServer.h"

#define SD_TEST_INTERVAL     100  // ms
#define MEMORY_TEST_INTERVAL 1000 // ms
#define STATUS_INTERVAL      333  // ms

using namespace MarvieXmlConfigParsers;

MarvieDevice* MarvieDevice::_inst = nullptr;

MarvieDevice::MarvieDevice() : configXmlDataSendingSemaphore( false )
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
	for( uint i = 0; i < MarviePlatform::comPortsCount; ++i )
	{
		usarts[i] = Usart::instance( MarviePlatform::comPortIoLines[i].sd );
		palSetPadMode( MarviePlatform::comPortIoLines[i].rxPort.gpio,
					   MarviePlatform::comPortIoLines[i].rxPort.pinNum,
					   usartAF( MarviePlatform::comPortIoLines[i].sd ) );
		palSetPadMode( MarviePlatform::comPortIoLines[i].txPort.gpio,
					   MarviePlatform::comPortIoLines[i].txPort.pinNum,
					   usartAF( MarviePlatform::comPortIoLines[i].sd ) );
		usarts[i]->setInputBuffer( comPortInputBuffers[i], COM_PORT_INPUT_BUFFER_SIZE );
		usarts[i]->setOutputBuffer( comPortOutputBuffers[i], COM_PORT_OUTPUT_BUFFER_SIZE );
		usarts[i]->open();
 		//if( MarviePlatform::comPortTypes[i] == ComPortType::Rs232 )
 			comPorts[i] = usarts[i];
 		/*else
 		{
 			palSetPadMode( MarviePlatform::comPortIoLines[i].rePort.gpio,
 						   MarviePlatform::comPortIoLines[i].rePort.pinNum,
 						   PAL_MODE_OUTPUT_PUSHPULL );
 			palSetPadMode( MarviePlatform::comPortIoLines[i].dePort.gpio,
 						   MarviePlatform::comPortIoLines[i].dePort.pinNum,
 						   PAL_MODE_OUTPUT_PUSHPULL );
 			comPorts[i] = new Rs485( usarts[i], MarviePlatform::comPortIoLines[i].rePort, MarviePlatform::comPortIoLines[i].dePort );
 		}*/
	}

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
	f_mount( &fatFs, "/", 0 );
	fsError = FR_NOT_READY;

	crcStart( &CRCD1, nullptr );

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

	mLinkServer = new MLinkServer;
	mLinkServer->setComplexDataReceiveCallback( this );
	mLinkServer->setIODevice( comPorts[2] );

	lwipInit( NULL );

	UdpSocket* socket = new UdpSocket;
	socket->bind( 16021 );
	socket->connect( IpAddress( 192, 168, 1, 12 ), 16032 );
	mLinkServer->setIODevice( socket );
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
	miskTasksThread          = Concurrent::run( [this]() { miskTasksThreadMain(); },          1024,  NORMALPRIO );
	mLinkServerHandlerThread = Concurrent::run( [this]() { mLinkServerHandlerThreadMain(); }, 1024,  NORMALPRIO );
	uiThread                 = Concurrent::run( [this]() { uiThreadMain(); },                 1024,  NORMALPRIO );

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
			{
				sdcDisconnect( &SDCD1 );
				sdCardStatus = SdCardStatus::NotInserted;
				removeOpenDatFiles();
			}
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
							sensor = reader->nextUpdatedSensor();
						}
					}
					else if( vPortBindings[i] == VPortBinding::Rs232 || vPortBindings[i] == VPortBinding::NetworkSocket )
						sensors[static_cast< SingleBRSensorReader* >( brSensorReaders[i] )->nextSensor()->userData()].updated = true;
					else
					{
						// TODO: add code here
					}
				}
			}
			if( updated )
				mLinkServerHandlerThread->signalEvents( MLinkThreadEvent::BrSensorUpdatedEvent );
		}
		if( em & MainThreadEvent::FormatSdCardRequest )
		{
			if( sdCardStatus != SdCardStatus::NotInserted && sdCardStatus != SdCardStatus::InitFailed )
			{
				sdCardStatus = SdCardStatus::Formatting;
				mLinkServerHandlerThread->signalEvents( MLinkThreadEvent::ConfigResetEvent );
				configMutex.lock();
				configShutdownM();
				deviceState = DeviceState::IncorrectConfiguration;
				configError = ConfigError::NoConfigFile;
				configMutex.unlock();

				removeOpenDatFiles();
				if( f_mkfs( "/", FM_FAT32, 1024, buffer, 1024 ) == FR_OK && f_mkdir( "/Temp" ) == FR_OK )
					sdCardStatus = SdCardStatus::Working;
				else
					sdCardStatus = SdCardStatus::BadFileSystem;
				configXmlHash = SHA1::Digest();
			}
		}
	}
}

void MarvieDevice::miskTasksThreadMain()
{
	virtual_timer_t sdTestTimer, memoryTestTimer;
	chVTObjectInit( &sdTestTimer );
	chVTObjectInit( &memoryTestTimer );
	chVTSet( &sdTestTimer, TIME_MS2I( SD_TEST_INTERVAL ), miskTaskThreadTimersCallback, ( void* )MiskTaskThreadEvent::SdCardTestEvent );
	chVTSet( &memoryTestTimer, TIME_MS2I( MEMORY_TEST_INTERVAL ), miskTaskThreadTimersCallback, ( void* )MiskTaskThreadEvent::MemoryTestEvent );

	while( true )
	{
		eventmask_t em = chEvtWaitAny( ALL_EVENTS );
		if( em & MiskTaskThreadEvent::SdCardTestEvent )
		{
			auto status = blkIsInserted( &SDCD1 );
			if( sdCardInserted != status )
			{
				sdCardInserted = status;
				mainThread->signalEvents( MainThreadEvent::SdCardStatusChanged );
			}
			chVTSet( &sdTestTimer, TIME_MS2I( SD_TEST_INTERVAL ), miskTaskThreadTimersCallback, ( void* )MiskTaskThreadEvent::SdCardTestEvent );
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
			chVTSet( &memoryTestTimer, TIME_MS2I( MEMORY_TEST_INTERVAL ), miskTaskThreadTimersCallback, ( void* )MiskTaskThreadEvent::MemoryTestEvent );
		}
	}
}

void MarvieDevice::mLinkServerHandlerThreadMain()
{
	EvtListener mLinkListener, cpuUsageMonitorListener;
	mLinkServer->eventSource()->registerMask( &mLinkListener, MLinkThreadEvent::MLinkEvent );
	CpuUsageMonitor::instance()->eventSource()->registerMask( &cpuUsageMonitorListener, MLinkThreadEvent::CpuUsageMonitorEvent );

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
					chVTSet( &timer, TIME_MS2I( STATUS_INTERVAL ), mLinkServerHandlerThreadTimersCallback, ( void* )MLinkThreadEvent::StatusUpdateEvent );
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
			mLinkServer->sendPacket( MarviePackets::Type::ConfigResetType, nullptr, 0 );
		if( em & MLinkThreadEvent::ConfigChangedEvent )
			mLinkSync( false );
		if( em & MLinkThreadEvent::BrSensorUpdatedEvent )
		{
			if( configMutex.tryLock() )
			{
				if( syncConfigNum == configNum )
				{
					for( uint i = 0; i < sensorsCount; ++i )
					{
						if( sensors[i].sensor->type() == AbstractSensor::Type::SR || !sensors[i].updated )
							continue;
						sensors[i].updated = false;
						sendSensorDataM( i, sensorDataChannel );
					}
				}
				sendDeviceStatusM();
				configMutex.unlock();
			}
		}
		if( em & MLinkThreadEvent::StatusUpdateEvent )
		{
			chVTSet( &timer, TIME_MS2I( STATUS_INTERVAL ), mLinkServerHandlerThreadTimersCallback, ( void* )MLinkThreadEvent::StatusUpdateEvent );
			if( configMutex.tryLock() )
			{
				for( uint i = 0; i < vPortsCount; ++i )
				{
					if( syncConfigNum != configNum )
						break;
					sendVPortStatusM( i );
				}
				sendDeviceStatusM();
				configMutex.unlock();
			}
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

void MarvieDevice::reconfig()
{
	configMutex.lock();

	auto hash = calcConfigXmlHash();
	if( hash == configXmlHash )
	{
		configMutex.unlock();
		return;
	}

	if( deviceState == DeviceState::Working )
	{
		deviceState = DeviceState::Reconfiguration;
		configShutdownM();
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
		applyConfigM( xmlData, size );
	}
	else
		configError = ConfigError::NoConfigFile;

	configMutex.unlock();
}

void MarvieDevice::configShutdownM()
{
	for( uint i = 0; i < vPortsCount; ++i )
	{
		if( brSensorReaders[i] )
			brSensorReaders[i]->stopReading();
	}
	for( uint i = 0; i < vPortsCount; ++i )
	{
		if( brSensorReaders[i] )
			brSensorReaders[i]->waitForStateChange();
	}

	removeConfigRelatedObjectM();
	mainThread->getAndClearEvents( MainThreadEvent::BrSensorReaderEvent );
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
	NetworkConf networkConf;
	if( !parseNetworkConfig( rootNode->FirstChildElement( "networkConfig" ), &networkConf ) )
	{
		deviceState = DeviceState::IncorrectConfiguration;
		configError = ConfigError::NetworkConfigError;

		delete doc;
		return;
	}
	ComPortConf** comPortsConf = parseComPortsConfig( rootNode->FirstChildElement( "comPortsConfig" ), comPortAssignments );
	if( !comPortsConf )
	{
		deviceState = DeviceState::IncorrectConfiguration;
		configError = ConfigError::ComPortsConfigError;

		delete doc;
		return;
	}

	vPortsCount = 0;
	for( uint i = 0; i < MarviePlatform::comPortsCount; ++i )
	{
		if( comPortsConf[i]->assignment() == ComPortAssignment::VPort )
		{
			++vPortsCount;
			comPorts[i]->setBaudRate( static_cast< VPortConf* >( comPortsConf[i] )->baudrate );
			comPorts[i]->setDataFormat( static_cast< VPortConf* >( comPortsConf[i] )->format );
		}

	}
	uint32_t ethernetVPortBegin = vPortsCount;
	vPortsCount += networkConf.vPortOverIpList.size();

	//for( uint i = 0; i <  )
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
			if( MarviePlatform::comPortTypes[n] == ComPortType::Rs232 )
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
				sensors[sensorsCount++].vPortId = vPortId;
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
						reader->setMinInterval( 0 );
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
				// TODO: add code here
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
		for( uint i = 0; i < vPortsCount; ++i )
		{
			brSensorReaderListeners[i].getAndClearFlags();
			if( brSensorReaders[i] )
			{
				brSensorReaders[i]->eventSource()->registerMask( brSensorReaderListeners + i, MainThreadEvent::BrSensorReaderEvent );
				brSensorReaders[i]->startReading( NORMALPRIO );
			}
		}
		deviceState = DeviceState::Working;
		mLinkServerHandlerThread->signalEvents( ( eventmask_t )MLinkThreadEvent::ConfigChangedEvent );
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
	{
		// TODO: 
		if( vPortBindings[i] == VPortBinding::Rs232 )
		{
			if( brSensorReaders[i] )
			{
				delete static_cast< SingleBRSensorReader* >( brSensorReaders[i] )->nextSensor();
			}
		}
		else if( vPortBindings[i] == VPortBinding::Rs485 )
		{
			if( brSensorReaders[i] )
				static_cast< MultipleBRSensorsReader* >( brSensorReaders[i] )->removeAllSensorElements( true );
		}
		else
		{
			// TODO: 
		}
		delete brSensorReaders[i];
	}

	delete vPorts;
	delete vPortBindings;
	delete sensors;
	delete brSensorReaders;
	delete brSensorReaderListeners;

	vPortsCount = 0;
	vPorts = nullptr;
	vPortBindings = nullptr;
	sensorsCount = 0;
	sensors = nullptr;
	brSensorReaders = nullptr;
	brSensorReaderListeners = nullptr;
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
		if( configMutex.tryLock() )
		{
			for( uint i = 0; i < vPortsCount; ++i )
			{
				if( brSensorReaders[i] )
					brSensorReaders[i]->startReading( NORMALPRIO );
			}
			configMutex.unlock();
		}
	}
	else if( type == MarviePackets::Type::StopVPortsType )
	{
		if( configMutex.tryLock() )
		{
			for( uint i = 0; i < vPortsCount; ++i )
			{
				if( brSensorReaders[i] )
					brSensorReaders[i]->stopReading();
			}
			configMutex.unlock();
		}
	}
	else if( type == MarviePackets::Type::UpdateAllSensorsType )
	{
		if( configMutex.tryLock() )
		{
			for( uint i = 0; i < vPortsCount; ++i )
			{
				if( brSensorReaders[i] )
					brSensorReaders[i]->forceAll();
			}
			configMutex.unlock();
		}
	}
	else if( type == MarviePackets::Type::UpdateOneSensorType )
	{
		if( configMutex.tryLock() )
		{
			uint32_t sensorId = *( uint16_t* )data;
			if( sensorsCount > sensorId && sensors[sensorId].sensor->type() == AbstractSensor::Type::BR )
			{
				auto reader = brSensorReaders[sensors[sensorId].vPortId];
				if( reader )
					reader->forceOne( reinterpret_cast< AbstractBRSensor* >( sensors[sensorId].sensor ) );
			}
			configMutex.unlock();
		}
	}
	else if( type == MarviePackets::Type::FormatSdCardType )
	{
		if( sdCardStatus != SdCardStatus::Formatting )
			mainThread->signalEvents( MainThreadEvent::FormatSdCardRequest );
	}
}

void MarvieDevice::mLinkSync( bool sendSupportedSensorsList )
{
	mLinkServer->sendPacket( MarviePackets::Type::SyncStartType, nullptr, 0 );
	auto channel = mLinkServer->createComplexDataChannel();
	if( sendSupportedSensorsList )
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

	if( configMutex.tryLock() )
	{
		if( syncConfigNum == configNum )
		{
			mLinkServer->sendPacket( MarviePackets::Type::VPortCountType, ( uint8_t* )&vPortsCount, sizeof( vPortsCount ) );
			for( uint i = 0; i < vPortsCount; ++i )
			{
				if( syncConfigNum != configNum )
					goto End;
				sendVPortStatusM( i );
			}
			sendDeviceStatusM();
			for( uint i = 0; i < sensorsCount; ++i )
			{
				if( syncConfigNum != configNum )
					goto End;
				sendSensorDataM( i, channel );
			}
		}
End:
		configMutex.unlock();
	}
	delete channel;
	mLinkServer->sendPacket( MarviePackets::Type::SyncEndType, nullptr, 0 );
}

void MarvieDevice::sendVPortStatusM( uint32_t vPortId )
{
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
	else
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

void MarvieDevice::sendDeviceStatusM()
{
	MarviePackets::DeviceStatus status;
	status.dateTime = DateTimeService::currentDateTime();
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
	mLinkServer->sendPacket( MarviePackets::Type::DeviceStatusType, ( uint8_t* )&status, sizeof( status ) );
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