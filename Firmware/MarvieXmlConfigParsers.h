#pragma once

#include "Lib/tinyxml2/tinyxml2.h"
#include "Drivers/Interfaces/UsartBasedDevice.h"
#include <list>

namespace MarvieXmlConfigParsers
{
	using namespace tinyxml2;

	enum class ComPortAssignment { VPort, GsmModem, ModbusRtuSlave, Multiplexer };

	struct ComPortConf
	{
		ComPortConf( ComPortAssignment a ) : a( a ) {}
		inline ComPortAssignment assignment() { return a; }

	private:
		ComPortAssignment a;
	};

	struct VPortOverIpConf
	{
		char ip[15 + 1];
		uint16_t port;
	};

	struct EthernetConf
	{
		uint16_t modbusRtuServerPort;
		uint16_t modbusTcpServerPort;
		uint16_t modbusAsciiServerPort;
		std::list< VPortOverIpConf > vPortOverIpList;
	};

	struct VPortConf : public ComPortConf
	{
		VPortConf() : ComPortConf( ComPortAssignment::VPort ) {}

		UsartBasedDevice::DataFormat format;
		uint32_t baudrate;
	};

	struct GsmModemConf : public ComPortConf
	{
		GsmModemConf() : ComPortConf( ComPortAssignment::GsmModem ) {}

		uint32_t pinCode;
		char vpn[20 + 1];
		uint16_t modbusRtuServerPort;
		uint16_t modbusTcpServerPort;
		uint16_t modbusAsciiServerPort;
		std::list< VPortOverIpConf > vPortOverIpList;
	};

	struct ModbusRtuSlaveConf : public ComPortConf
	{
		ModbusRtuSlaveConf() : ComPortConf( ComPortAssignment::ModbusRtuSlave ) {}

		UsartBasedDevice::DataFormat format;
		uint32_t baudrate;
		uint8_t address;
	};

	struct  MultiplexerConf : public ComPortConf
	{
		MultiplexerConf() : ComPortConf( ComPortAssignment::Multiplexer ) {}

		UsartBasedDevice::DataFormat format[5];
		uint32_t baudrate[5];
	};

	UsartBasedDevice::DataFormat toFrameFormat( const char* s );
	bool parseVPortConfig( XMLElement* node, VPortConf* conf );
	bool parseVPortOverIp( XMLElement* node, VPortOverIpConf* conf );
	bool parseVPortOverIpGroup( XMLElement* node, std::list< VPortOverIpConf >* vPortOverIpList );
	bool parseGsmModemConfig( XMLElement* node, GsmModemConf* conf );
	bool parseModbusRtuSlaveConfig( XMLElement* node, ModbusRtuSlaveConf* conf );
	bool parseMultiplexerConfig( XMLElement* node, MultiplexerConf* conf );
	ComPortConf** parseComPortsConfig( XMLElement* comPortsConfigNode, const std::list< std::list< ComPortAssignment > >& comPortAssignments );
	bool parseEthernetConfig( XMLElement* ethernetConfigNode, EthernetConf* conf );
}