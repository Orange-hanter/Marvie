#pragma once

#include "Core/DateTime.h"

namespace MarviePackets
{
	enum Type : uint8_t { FirmwareDescType, CpuLoadType, MemoryLoadType, VPortStatusType, GetConfigXmlType, ConfigXmlMissingType, SyncStartType, SyncEndType, ConfigResetType, VPortCountType, DeviceStatusType, EthernetStatusType, GsmStatusType, ServiceStatisticsType, SensorErrorReportType, SetDateTimeType, EjectSdCardType, FormatSdCardType, StopVPortsType, StartVPortsType, UpdateAllSensorsType, UpdateOneSensorType, RestartDeviceType, CleanMonitoringLogType, CleanSystemLogType, AnalogInputsDataType, DigitInputsDataType };
	enum ComplexChannel { BootloaderChannel, FirmwareChannel, XmlConfigChannel, XmlConfigSyncChannel, SensorDataChannel, SupportedSensorsListChannel };

	struct FirmwareDesc
	{
		char coreVersion[15 + 1];
		char targetName[25 + 1];
	};
	struct CpuLoad 
	{
		float load;
	};
	struct MemoryLoad
	{
		uint32_t totalGRam;
		uint32_t totalCcRam;

		uint32_t freeGRam;
		uint32_t gRamHeapSize;
		uint32_t gRamHeapFragments;
		uint32_t gRamHeapLargestFragmentSize;

		uint32_t freeCcRam;
		uint32_t ccRamHeapSize;
		uint32_t ccRamHeapFragments;
		uint32_t ccRamHeapLargestFragmentSize;

		unsigned long long sdCardCapacity;
		unsigned long long sdCardFreeSpace;
		unsigned long long logSize;
	};
	struct VPortStatus
	{
		uint8_t vPortId;
		enum class State : uint8_t { Stopped, Working, Stopping } state;
		int16_t sensorId; // -1 if there is no sensor
		uint32_t timeLeft;
	};
	struct VPortCount
	{
		uint32_t count;
	};
	struct DeviceStatus
	{
		DateTime dateTime;
		enum class DeviceState : uint8_t { Reconfiguration, Working, IncorrectConfiguration } state;
		enum class ConfigError : uint8_t { NoError, NoConfiglFile, XmlStructureError, ComPortsConfigError, NetworkConfigError, SensorReadingConfigError, LogConfigError, SensorsConfigError } configError;
		enum class SdCardStatus : uint8_t { NotInserted, Initialization, InitFailed, BadFileSystem, Formatting, Working } sdCardStatus;
		enum class LogState : uint8_t { Off, Stopped, Working, Archiving } logState;
		uint8_t errorSensorId;
	};
	struct EthernetStatus
	{
		uint32_t ip;
		uint32_t netmask;
		uint32_t gateway;
	};
	struct GsmStatus
	{
		enum class State { Off, Stopped, Initializing, Working, Stopping } state;
		enum class Error { NoError, AuthenticationError, TimeoutError, UnknownError } error;
		uint32_t ip;
	};
	struct SensorErrorReport 
	{
		uint8_t sensorId;
		uint8_t vPortId;
		enum class Error : uint8_t { CrcError, NoResponseError } error;
		uint8_t errorCode;
		DateTime dateTime;
	};
	struct ServiceStatistics
	{
		int16_t rawModbusClientsCount;
		int16_t tcpModbusRtuClientsCount;
		int16_t tcpModbusAsciiClientsCount;
		int16_t tcpModbusIpClientsCount;
	};
};