#pragma once

#include "Core/ObjectMemoryUtilizer.h"
#include "Core/CpuUsageMonitor.h"
#include "Core/Concurrent.h"
#include "Core/Assert.h"
#include "Drivers/Interfaces/Usart.h"
#include "Drivers/Interfaces/Rs485.h"
#include "SensorService/SensorService.h"
#include "MarvieXmlConfigParsers.h"
#include "MultipleBRSensorsReader.h"
#include "SingleBRSensorReader.h"
#include "MarviePackets.h"
#include "MLinkServer.h"
#include "MarviePlatform.h"
#include "Lib/sha1/sha1.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>

#define COM_PORT_INPUT_BUFFER_SIZE   1024
#define COM_PORT_OUTPUT_BUFFER_SIZE  1024

class MarvieDevice : public MLinkServer::ComplexDataCallback
{
	MarvieDevice();

public:
	static MarvieDevice* instance();

	void exec();

private:
	void mainThreadMain();
	void miskTasksThreadMain();
	void mLinkServerHandlerThreadMain();
	void uiThreadMain();

	void reconfig();
	void applyConfig( char* xmlData, uint32_t len );
	void removeConfigRelatedObject();
	void removeConfigRelatedObjectM();

	uint32_t onOpennig( uint8_t id, const char* name, uint32_t size ) final override;
	bool newDataReceived( uint8_t id, const uint8_t* data, uint32_t size ) final override;
	void onClosing( uint8_t id, bool canceled ) final override;

	void mLinkProcessNewPacket( uint32_t type, uint8_t* data, uint32_t size );
	void mLinkSync( bool sendSupportedSensorsList );
	void sendVPortStatusM( uint32_t vPortId );
	void sendSensorDataM( uint32_t sensorId, MLinkServer::ComplexDataChannel* channel );
	void sendDeviceStatusM();

	uint32_t readConfigXmlFile( char** pXmlData );
	FRESULT clearDir( const char* path );
	void removeOpenDatFiles();

	static void miskTaskThreadTimersCallback( void* p );
	static void mLinkServerHandlerThreadTimersCallback( void* p );

private:
	static MarvieDevice* _inst;
	std::list< std::list< MarvieXmlConfigParsers::ComPortAssignment > > comPortAssignments;
	Usart* usarts[MarviePlatform::comPortsCount];
	UsartBasedDevice* comPorts[MarviePlatform::comPortsCount];
	uint8_t comPortInputBuffers[MarviePlatform::comPortsCount][COM_PORT_INPUT_BUFFER_SIZE];
	uint8_t comPortOutputBuffers[MarviePlatform::comPortsCount][COM_PORT_OUTPUT_BUFFER_SIZE];
	uint32_t buffer[1024];
	SHA1 sha;

	// ======================================================================================
	Mutex configMutex;
	uint32_t configNum;
	uint32_t vPortsCount;
	IODevice** vPorts;
	enum class VPortBindind { Rs232, Rs485, NetworkSocket } *vPortBindings;
	uint32_t sensorsCount;
	struct SensorInfo 
	{
		AbstractSensor* sensor;
		uint16_t vPortId;
		volatile bool updated;
	}* sensors;
	BRSensorReader** brSensorReaders;
	EvtListener* brSensorReaderListeners;
	// ======================================================================================
	
	BaseDynamicThread* mainThread, *miskTasksThread, *mLinkServerHandlerThread, *uiThread;
	volatile bool mLinkComPortEnable = true;  // shared resource
	Mutex configXmlFileMutex;

	// Main thread resources =================================================================
	enum MainThreadEvent { SdCardStatusChanged = 1, NewBootloaderDatFile = 2, NewFirmwareDatFile = 4, NewXmlConfigDatFile = 8, BrSensorReaderEvent = 16 };
	enum class DeviceState { /*Initialization,*/ Reconfiguration, Working, IncorrectConfiguration } deviceState;
	enum class ConfigError { NoError, XmlStructureError, ComPortsConfigError, EthernetConfigError, SensorsConfigError } configError;
	enum class SensorsConfigError { NoError, UnknownSensor, IncorrectSettings, BingingError } sensorsConfigError;
	const char* errorSensorName;
	enum class SdCardStatus { NotInserted, Initialization, InitFailed, BadFileSystem, Formatting, Working } sdCardStatus;
	FATFS fatFs;
	FRESULT fsError;	
	SHA1::Digest configXmlHash;

	// MLink thread resources ================================================================
	enum MLinkThreadEvent : eventmask_t { MLinkEvent = 1, CpuUsageMonitorEvent = 2, MemoryLoadEvent = 4, ConfigChanged = 8, BrSensorUpdated = 16, StatusUpdate = 32 };
	MLinkServer* mLinkServer;
	uint8_t mLinkBuffer[255];
	Mutex datFilesMutex;
	FIL* datFiles[3]; // shared resource
	uint32_t syncConfigNum;
	BinarySemaphore configXmlDataSendingSemaphore;

	// Misk tasks thread resources ===========================================================
	enum MiskTaskThreadEvent : eventmask_t { SdCardTestEvent = 1, MemoryTestEvent = 2 };
	volatile bool sdCardInserted;
	MarviePackets::MemoryLoad memoryLoad;
	// UI thread resources ===================================================================
};