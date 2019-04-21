#pragma once

#include "Core/IOPort.h"
#include "MarvieXmlConfigParsers.h"
#include "hal.h"
#include "stdint.h"
#include <list>

enum class ComPortType { Rs232, Rs485 };

class MarviePlatform
{
public:
	static constexpr const char* coreVersion = "0.8.2.1L";
	static constexpr const char* platformType = "VX";
	static constexpr const char* configXmlRootTagName = "vxConfig";

	static const IOPort analogInputAddressSelectorPorts[4];
	static constexpr uint32_t analogInputsCount = 16;

	static constexpr uint32_t digitInputsCount = 8;
	static const IOPort digitInputPorts[digitInputsCount];

	static constexpr uint32_t srSensorUpdatePeriodMs = 50;

	static constexpr uint32_t comUsartsCount = 4;
	struct ComUsartIoLines
	{
		SerialDriver* sd;
		IOPort rxPort, txPort;
	};
	static const ComUsartIoLines comUsartIoLines[comUsartsCount];

	static constexpr uint32_t comPortsCount = 7;
	static constexpr uint32_t mLinkComPort = 6;
	static constexpr uint32_t gsmModemComPort = 0;
	static const IOPort gsmModemEnableIoPort;
	static constexpr const bool gsmModemEnableLevel = 0;
	static const ComPortType comPortTypes[comPortsCount];
	struct ComPortIoLines
	{
		uint32_t comUsartIndex;
		IOPort rePort, dePort;
	};
	static const ComPortIoLines comPortIoLines[comPortsCount];
	static void comPortAssignments( std::list< std::list< MarvieXmlConfigParsers::ComPortAssignment > >& comPortAssignments );
};
