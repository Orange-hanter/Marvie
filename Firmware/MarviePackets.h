#pragma once

#include "Core/DateTime.h"

namespace MarviePackets
{
	enum Type : uint8_t { CpuLoadType, MemoryLoadType, VPortStatusType, GetConfigXmlType, ConfigXmlMissingType, SyncStartType, SyncEndType, VPortCountType, DeviceStatusType, SensorErrorReportType, SetDateTimeType, StopVPorts, StartVPorts, UpdateAllSensors, UpdateOneSensor };
	enum ComplexChannel { BootloaderChannel, FirmwareChannel, XmlConfigChannel, XmlConfigSyncChannel, SensorDataChannel, SupportedSensorsListChannel };

	struct CpuLoad 
	{
		float load;
	};
	struct MemoryLoad
	{
		uint32_t totalRam;
		uint32_t staticAllocatedRam;
		uint32_t heapAllocatedRam;
		unsigned long long sdCardCapacity;
		unsigned long long sdCardFreeSpace;
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
		enum class DeviceState { Reconfiguration, Working, IncorrectConfiguration } state;
	};
	struct SensorErrorReport 
	{
		uint8_t sensorId;
		uint8_t vPortId;
		enum class Error : uint8_t { CrcError, NoResponseError } error;
		uint8_t errorCode;
		DateTime dateTime;
	};
};