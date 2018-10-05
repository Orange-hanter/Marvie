#pragma once

#include "Core/ObjectMemoryUtilizer.h"
#include "Core/RtcBackupRegisters.h"
#include "Core/CpuUsageMonitor.h"
#include "Core/Concurrent.h"
#include "Core/Assert.h"
#include "Drivers/Interfaces/Usart.h"
#include "Drivers/Interfaces/Rs485.h"
#include "Drivers/Interfaces/SharedRs485.h"
#include "Drivers/Network/Ethernet/EthernetThread.h"
#include "Drivers/Network/SimGsm/SimGsmPppModem.h"
#include "SensorService/SensorService.h"
#include "MarvieXmlConfigParsers.h"
#include "MultipleBRSensorsReader.h"
#include "SingleBRSensorReader.h"
#include "NetworkSensorReader.h"
#include "MarviePackets.h"
#include "MLinkServer.h"
#include "MarviePlatform.h"
#include "Lib/sha1/_Sha1.h"
#include "ff.h"
#include "Apps/Modbus/RawModbusServer.h"
#include "Apps/Modbus/TcpModbusServer.h"
#include "Log/MarvieLog.h"
#include "Log/FileLog.h"
#include <stdio.h>
#include <string.h>

#define COM_USART_INPUT_BUFFER_SIZE   1024
#define COM_USART_OUTPUT_BUFFER_SIZE  1024
#define GSM_MODEM_OUTPUT_BUFFER       4096

class MarvieDevice : private AbstractSRSensor::SignalProvider, private MLinkServer::ComplexDataCallback, private ModbusPotato::ISlaveHandler
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
	void uiThreadMain();

	void logFailure();
	void ejectSdCard();
	inline void formatSdCard();

	void reconfig();
	void configShutdown();
	void applyConfigM( char* xmlData, uint32_t len );
	void removeConfigRelatedObject();
	void removeConfigRelatedObjectM();

	void copySensorDataToModbusRegisters( AbstractSensor* sensor );

	float analogSignal( uint32_t block, uint32_t line ) final override;
	bool digitSignal( uint32_t block, uint32_t line ) final override;
	uint32_t digitSignals( uint32_t block ) final override;

	uint32_t onOpennig( uint8_t id, const char* name, uint32_t size ) final override;
	bool newDataReceived( uint8_t id, const uint8_t* data, uint32_t size ) final override;
	void onClosing( uint8_t id, bool canceled ) final override;

	void mLinkProcessNewPacket( uint32_t type, uint8_t* data, uint32_t size );
	void mLinkSync( bool coldSync );
	void sendFirmwareDesc();
	void sendVPortStatusM( uint32_t vPortId );
	void sendSensorDataM( uint32_t sensorId, MLinkServer::ComplexDataChannel* channel );
	void sendDeviceStatus();
	void sendEthernetStatus();
	void sendGsmModemStatusM();
	void sendServiceStatisticsM();
	void sendAnalogInputsData();
	void sendDigitInputsData();

	uint32_t readConfigXmlFile( char** pXmlData );
	SHA1::Digest calcConfigXmlHash();
	FRESULT clearDir( const char* path );
	void removeOpenDatFiles();

	ModbusPotato::modbus_exception_code::modbus_exception_code read_input_registers( uint16_t, uint16_t, uint16_t* ) override;

	static void mainTaskThreadTimersCallback( void* p );
	static void miskTaskThreadTimersCallback( void* p );
	static void mLinkServerHandlerThreadTimersCallback( void* p );
	static void adInputsReadThreadTimersCallback( void* p );
	static void adcCallback( ADCDriver *adcp, adcsample_t *buffer, size_t n );
	static void adcErrorCallback( ADCDriver *adcp, adcerror_t err );

public:
	static void faultHandler();

private:
	static MarvieDevice* _inst;
	enum Interval // in ms
	{
		SdTestInterval            = 100,
		MemoryTestInterval        = 1000,
		StatusInterval            = 333,
		SrSensorInterval          = 100,
		SrSensorPeriodCounterLoad = 1000 / SrSensorInterval,
	};
	enum ThreadPriority : tprio_t
	{
		EthernetThreadPrio = NORMALPRIO + 1
	};
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
	uint32_t buffer[1024];
	SHA1 sha;
	struct SettingsBackup
	{
		struct Flags
		{
			uint32_t ethernetDhcp : 1;
		} flags;
		struct Ethernet
		{
			IpAddress ip;
			uint32_t netmask;
			IpAddress gateway;
		} eth;
	};
	struct FailureDescBackup
	{
		enum Type : uint32_t
		{
			None = 0,
			Reset = 1,
			NMI = 2,
			HardFault = 3,
			MemManage = 4,
			BusFault = 5,
			UsageFault = 6,
		} type;
		uint32_t flags;
		uint32_t busAddress;
		uint32_t pc;
		uint32_t lr;
	};
	enum
	{
		SettingsBackupOffset = 0,
		FailureDescBackupOffset = sizeof( SettingsBackup ) / 4 + 1
	};
	RtcBackupRegisters settingsBackupRegs;
	char apn[25] = {};
	FileLog fileLog;

	// ======================================================================================
	Mutex configMutex;
	uint32_t configNum;
	uint32_t vPortsCount;
	IODevice** vPorts;
	enum class VPortBinding { Rs232, Rs485, NetworkSocket } *vPortBindings;
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
	EvtListener* brSensorReaderListeners;
	MarvieLog* marvieLog;
	volatile bool sensorLogEnabled;
	AbstractPppModem* gsmModem;
	EvtListener gsmModemListener;
	RawModbusServer* rawModbusServers;
	uint32_t rawModbusServersCount;
	TcpModbusServer* tcpModbusRtuServer;
	TcpModbusServer* tcpModbusAsciiServer;
	TcpModbusServer* tcpModbusIpServer;
	Mutex modbusRegistersMutex;
	uint16_t* modbusRegisters;
	uint32_t modbusRegistersCount;
	// ======================================================================================
	
	BaseDynamicThread* mainThread, *miskTasksThread, *adInputsReadThread, *mLinkServerHandlerThread, *uiThread;
	volatile bool mLinkComPortEnable = true;  // shared resource
	Mutex configXmlFileMutex;

	// Main thread resources =================================================================
	enum MainThreadEvent { SdCardStatusChanged = 1, NewBootloaderDatFile = 2, NewFirmwareDatFile = 4, NewXmlConfigDatFile = 8, StartSensorReaders = 16, StopSensorReaders = 32, BrSensorReaderEvent = 64, SrSensorsTimerEvent = 128, EjectSdCardRequest = 256, FormatSdCardRequest = 512, CleanMonitoringLogRequestEvent = 1024, CleanSystemLogRequestEvent = 2048, RestartRequestEvent = 4096 };
	enum class DeviceState { /*Initialization,*/ Reconfiguration, Working, IncorrectConfiguration } deviceState;
	enum class ConfigError { NoError, NoConfigFile, XmlStructureError, ComPortsConfigError, NetworkConfigError, SensorReadingConfigError, LogConfigError, SensorsConfigError } configError;
	enum class SensorsConfigError { NoError, UnknownSensor, IncorrectSettings, BingingError } sensorsConfigError;
	const char* errorSensorTypeName;
	uint32_t errorSensorId;
	enum class SdCardStatus { NotInserted, Initialization, InitFailed, BadFileSystem, Formatting, Working } sdCardStatus;
	FATFS fatFs;
	FRESULT fsError;
	SHA1::Digest configXmlHash;
	Timer srSensorsUpdateTimer;
	uint32_t srSensorPeriodCounter;
	uint64_t monitoringLogSize;

	// MLink thread resources ================================================================
	enum MLinkThreadEvent : eventmask_t { MLinkEvent = 1, CpuUsageMonitorEvent = 2, MemoryLoadEvent = 4, EthernetEvent = 8, GsmModemEvent = 16, ConfigChangedEvent = 32, ConfigResetEvent = 64, SensorUpdateEvent = 128, StatusUpdateEvent = 256 };
	MLinkServer* mLinkServer;
	uint8_t mLinkBuffer[255];
	Mutex datFilesMutex;
	FIL* datFiles[3]; // shared resource
	uint32_t syncConfigNum;
	BinarySemaphore configXmlDataSendingSemaphore;

	// Misk tasks thread resources ===========================================================
	enum MiskTaskThreadEvent : eventmask_t { SdCardTestEvent = 1, MemoryTestEvent = 2 };
	volatile bool sdCardInserted;
	volatile bool sdCardStatusEventPending;
	MarviePackets::MemoryLoad memoryLoad;
	// UI thread resources ===================================================================
};