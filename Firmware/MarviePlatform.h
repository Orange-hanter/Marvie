#pragma once

#include "Core/IOPort.h"
#include "MarvieXmlConfigParsers.h"
#include "ch.h"
#include "stdint.h"
#include <list>

enum class ComPortType { Rs232, Rs485 };

class MarviePlatform
{
public:
	//static constexpr bool ethernetEnable = false;
	static constexpr uint32_t comPortsCount = 3;
	static constexpr uint32_t mLinkComPort = 2;
	static const ComPortType comPortTypes[comPortsCount];
	struct ComPortIoLines
	{
		SerialDriver* sd;
		IOPort rxPort, txPort, rePort, dePort;
	};
	static const ComPortIoLines comPortIoLines[comPortsCount];
	static const char* configXmlRootTagName;
	static void comPortAssignments( std::list< std::list< MarvieXmlConfigParsers::ComPortAssignment > >& comPortAssignments );
};