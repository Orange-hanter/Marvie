#pragma once

#include "Apps/Modbus/RawModbusServer.h"
#include "Apps/Modbus/TcpModbusServer.h"
#include "Core/Assert.h"
#include "Core/Concurrent.h"
#include "Core/CpuUsageMonitor.h"
#include "Core/Mutex.h"
#include "Core/ObjectMemoryUtilizer.h"
#include "Core/Semaphore.h"
#include "Core/Thread.h"
#include "Core/Timer.h"
#include "Drivers/Interfaces/Rs485.h"
#include "Drivers/Interfaces/SharedRs485.h"
#include "Drivers/Interfaces/Usart.h"
#include "Drivers/Network/Ethernet/EthernetThread.h"
#include "Drivers/Network/SimGsm/SimGsmPppModem.h"
#include "FirmwareTransferService.h"
#include "Log/FileLog.h"
#include "Log/MarvieLog.h"
#include "MLinkServer.h"
#include "MarvieBackup.h"
#include "MarviePackets.h"
#include "MarviePlatform.h"
#include "MarvieXmlConfigParsers.h"
#include "MultipleBRSensorsReader.h"
#include "NetworkSensorReader.h"
#include "RemoteTerminal/RemoteTerminalServer.h"
#include "SensorService/SensorService.h"
#include "SingleBRSensorReader.h"
#include "_Sha1.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>

#define COM_USART_INPUT_BUFFER_SIZE   1024
#define COM_USART_OUTPUT_BUFFER_SIZE  1024
#define GSM_MODEM_OUTPUT_BUFFER       4096

typedef unsigned int uint;

class MarvieDevice : private FirmwareTransferService::Callback, private AbstractSRSensor::SignalProvider, private MLinkServer::DataChannelCallback, private MLinkServer::AuthenticationCallback, private ModbusPotato::ISlaveHandler
{
	MarvieDevice();

public:
	static MarvieDevice* instance();

	void exec();

private:
	void mainThreadMain();
	void miskTasksThreadMain();
	void adInputsReadThreadMain();
	void mLinkServerHandlerThreadMain();
	void terminalOutputThreadMain();
	void uiThreadMain();
	void networkServiceThreadMain();

	void createGsmModemObjectM();
	void configGsmModemUsart();
	void logFailure();
	void ejectSdCard();
	void formatSdCard();
	std::unique_ptr< File > findBootloaderFile();
	void updateBootloader( File* file );

	void reconfig();
	void configShutdown();
	void applyConfigM( char* xmlData, uint32_t len );
	void removeConfigRelatedObject();
	void removeConfigRelatedObjectM();

	UsartBasedDevice* startComPortSharing( uint32_t index );
	void stopComPortSharing();

	void networkSharedComPortThreadMain( UsartBasedDevice* ioDevice, MarviePackets::ComPortSharingSettings::Mode mode );

	const char* firmwareVersion() override;
	const char* bootloaderVersion() override;
	void firmwareDownloaded( const std::string& fileName ) override;
	void bootloaderDownloaded( const std::string& fileName ) override;
	void restartDevice() override;

	void copySensorDataToModbusRegisters( AbstractSensor* sensor );

	float analogSignal( uint32_t block, uint32_t line ) final override;
	bool digitSignal( uint32_t block, uint32_t line ) final override;
	uint32_t digitSignals( uint32_t block ) final override;

	int authenticate( char* accountName, char* password ) final override;

	bool onOpennig( uint8_t channel , const char* name, uint32_t size ) final override;
	bool newDataReceived( uint8_t channel, const uint8_t* data, uint32_t size ) final override;
	void onClosing( uint8_t channel, bool canceled ) final override;

	static void terminalOutput( const uint8_t* data, uint32_t size, void* p );

	void mLinkProcessNewPacket( uint32_t type, uint8_t* data, uint32_t size );
	void mLinkSync( bool coldSync );
	void sendFirmwareDesc();
	void sendSupportedSensorsList();
	void sendDeviceSpec();
	void sendVPortStatusM( uint32_t vPortId );
	void sendSensorDataM( uint32_t sensorId, MLinkServer::DataChannel* channel );
	void sendDeviceStatus();
	void sendEthernetStatus();
	void sendGsmModemStatusM();
	void sendServiceStatisticsM();
	void sendComPortSharingStatus();
	void sendAnalogInputsData();
	void sendDigitInputsData();

	uint32_t readConfigXmlFile( char** pXmlData );
	SHA1::Digest calcConfigXmlHash();
	FRESULT clearDir( const char* path );
	void removeOpenDatFiles();

	ModbusPotato::modbus_exception_code::modbus_exception_code read_input_registers( uint16_t, uint16_t, uint16_t* ) override;

	static void mainTaskThreadTimersCallback( eventmask_t emask );
	static void miskTaskThreadTimersCallback( eventmask_t emask );
	static void mLinkServerHandlerThreadTimersCallback( eventmask_t emask );
	void terminalOutputTimerCallback();
	static void adInputsReadThreadTimersCallback( eventmask_t emask );
	static void adcCallback( ADCDriver *adcp, adcsample_t *buffer, size_t n );
	static void adcErrorCallback( ADCDriver *adcp, adcerror_t err );

public:
	static void powerDownExtiCallback( void* );
	static void faultHandler();
	static void systemHaltHook( const char* reason );

private:
	static int rtc( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] );
	static int lgbt( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] );
	static int qAsciiArtFunction( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] );

private:
	static MarvieDevice* _inst;
	enum Interval // in ms
	{
		SdTestInterval            = 100,
		MemoryTestInterval        = 1000,
		StatusInterval            = 500,
		SrSensorInterval          = 100,
		SrSensorPeriodCounterLoad = 1000 / SrSensorInterval,
	};
	enum ThreadPriority : tprio_t
	{
		EthernetThreadPrio = NORMALPRIO + 1
	};
	volatile bool incorrectShutdown;
	DateTime startDateTime;
	std::list< std::list< MarvieXmlConfigParsers::ComPortAssignment > > comPortAssignments;
	LogicOutput analogInputAddress[4];
	adcsample_t adcSamples[8];
	float analogInput[MarviePlatform::analogInputsCount] = {};
	uint32_t digitInputs;
	thread_reference_t adInputsReadThreadRef;
	struct ComUsart
	{
		ComUsart() { usart = nullptr; sharedRs485Control = nullptr; }
		Usart* usart;
		SharedRs485Control* sharedRs485Control;
	} comUsarts[MarviePlatform::comUsartsCount];
	UsartBasedDevice* comPorts[MarviePlatform::comPortsCount];
	uint8_t* comUsartInputBuffers[MarviePlatform::comUsartsCount];
	uint8_t* comUsartOutputBuffers[MarviePlatform::comUsartsCount];
	uint8_t buffer[1024];
	SHA1 sha;
	FileLog fileLog;

	// ======================================================================================
	Mutex configMutex;
	uint32_t configNum;
	MarvieXmlConfigParsers::ComPortAssignment comPortCurrentAssignments[MarviePlatform::comPortsCount];
	void* comPortServiceRoutines[MarviePlatform::comPortsCount];
	bool comPortBlockFlags[MarviePlatform::comPortsCount];
	uint32_t vPortsCount;
	IODevice** vPorts;
	enum class VPortBinding { Rs232, Rs485, NetworkSocket } *vPortBindings;
	int vPortToComPortMap[MarviePlatform::comPortsCount];
	uint32_t sensorsCount;
	struct SensorInfo
	{
		AbstractSensor* sensor;
		std::string sensorName;
		uint16_t modbusStartRegNum;
		uint16_t vPortId;
		uint32_t logPeriod;
		uint32_t logTimeLeft;
		DateTime dateTime;
		volatile bool updatedForMLink;
		volatile bool readyForMLink;
		volatile bool readyForLog;
	} *sensors;
	BRSensorReader** brSensorReaders;
	EventListener* brSensorReaderListeners;
	MarvieLog* marvieLog;
	volatile bool sensorLogEnabled;
	AbstractPppModem* gsmModem;
	RawModbusServer* rawModbusServers;
	uint32_t rawModbusServersCount;
	TcpModbusServer* tcpModbusRtuServer;
	TcpModbusServer* tcpModbusAsciiServer;
	TcpModbusServer* tcpModbusIpServer;
	Mutex modbusRegistersMutex;
	uint16_t* modbusRegisters;
	uint32_t modbusRegistersCount;
	Mutex sharingComPortMutex;
	ThreadsQueue sharingComPortThreadQueue;
	int sharedComPortIndex;
	// ======================================================================================

	Thread* mainThread, *miskTasksThread, *adInputsReadThread, *mLinkServerHandlerThread, *terminalOutputThread, *uiThread, *networkServiceThread;
	volatile bool mLinkComPortEnable = true;  // shared resource
	Mutex configXmlFileMutex;

	// Main thread resources =================================================================
	enum MainThreadEvent : eventmask_t 
	{
		SdCardStatusChanged = 1 << 0, NewBootloaderDatFile = 1 << 1, NewFirmwareDatFile = 1 << 2, NewXmlConfigDatFile = 1 << 3,
		PowerDownDetected = 1 << 4, StartSensorReaders = 1 << 5, StopSensorReaders = 1 << 6, BrSensorReaderEvent = 1 << 7,
		SrSensorsTimerEvent = 1 << 8, EjectSdCardRequest = 1 << 9, FormatSdCardRequest = 1 << 10, CleanMonitoringLogRequestEvent = 1 << 11,
		CleanSystemLogRequestEvent = 1 << 12, RestartRequestEvent = 1 << 13, GsmModemMainEvent = 1 << 14, RestartGsmModemRequestEvent = 1 << 15,
		StartComPortSharingEvent = 1 << 16, StopComPortSharingEvent = 1 << 17
	};
	enum class DeviceState { /*Initialization,*/ Reconfiguration, Working, IncorrectConfiguration } deviceState;
	enum class ConfigError { NoError, NoConfigFile, XmlStructureError, ComPortsConfigError, NetworkConfigError, DateTimeConfigError, SensorReadingConfigError, LogConfigError, SensorsConfigError } configError;
	enum class SensorsConfigError { NoError, UnknownSensor, IncorrectSettings, BingingError } sensorsConfigError;
	const char* errorSensorTypeName;
	uint32_t errorSensorId;
	enum class SdCardStatus { NotInserted, Initialization, InitFailed, BadFileSystem, Formatting, Working } sdCardStatus;
	FATFS fatFs;
	FRESULT fsError;
	SHA1::Digest configXmlHash;
	BasicTimer< decltype( &MarvieDevice::mainTaskThreadTimersCallback ), &MarvieDevice::mainTaskThreadTimersCallback > srSensorsUpdateTimer;
	uint32_t srSensorPeriodCounter;
	uint64_t monitoringLogSize;
	EventListener gsmModemMainThreadListener;
	FirmwareTransferService firmwareTransferService;

	// MLink thread resources ================================================================
	enum MLinkThreadEvent : eventmask_t 
	{
		MLinkEvent = 1, CpuUsageMonitorEvent = 2, MemoryLoadEvent = 4, EthernetEvent = 8,
		/*MLinkTcpServerEvent = 16, MLinkTcpSocketEvent = 32,*/ GsmModemMLinkEvent = 64, ConfigChangedEvent = 128,
		ConfigResetEvent = 256, SensorUpdateEvent = 512, StatusUpdateEvent = 1024, NetworkSharedComPortThreadFinishedEvent = 2048
	};
	MLinkServer* mLinkServer;
	uint8_t mLinkBuffer[255];
	EventListener gsmModemMLinkThreadListener;
	Mutex datFilesMutex;
	FIL* datFiles[3]; // shared resource
	uint32_t syncConfigNum;
	BinarySemaphore configXmlDataSendingSemaphore;
	Thread* networkSharedComPortThread;
	enum NetworkSharedComPortThread : eventmask_t { StopNetworkSharedComPortThreadRequestEvent = 1 };
	int16_t sharedComPortNetworkClientsCount;

	// TerminalOutput thread resources ================================================================
	enum TerminalOutputThreadEvent : eventmask_t { TerminalOutputEvent = 1 };
	RemoteTerminalServer terminalServer;
	BasicTimer< decltype( &MarvieDevice::terminalOutputTimerCallback ), &MarvieDevice::terminalOutputTimerCallback > terminalOutputTimer;
	StaticRingBuffer< uint8_t, 1024 > terminalOutputBuffer;

	// Misk tasks thread resources ===========================================================
	enum MiskTaskThreadEvent : eventmask_t { SdCardTestEvent = 1, MemoryTestEvent = 2 };
	volatile bool sdCardInserted;
	volatile bool sdCardStatusEventPending;
	MarviePackets::MemoryLoad memoryLoad;

	// Network test thread resources ===================================================================
	enum NetworkServiceThreadEvent : eventmask_t
	{ 
		GsmModemNetworkServiceEvent = 1, CheckGsmModemEvent = 2, CheckNtpServerAddrEvent = 4, ConfigChangedNetworkServiceEvent = 8,
	};
	IpAddress testIpAddr;
	EventListener gsmModemNetworkServiceThreadListener;

	// UI thread resources ===================================================================
};