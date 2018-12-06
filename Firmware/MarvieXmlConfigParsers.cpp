#include "MarvieXmlConfigParsers.h"

UsartBasedDevice::DataFormat MarvieXmlConfigParsers::toFrameFormat( const char* s )
{
	if( strcmp( s, "B8N" ) == 0 )
		return UsartBasedDevice::B8N;
	if( strcmp( s, "B7E" ) == 0 )
		return UsartBasedDevice::B7E;
	if( strcmp( s, "B7O" ) == 0 )
		return UsartBasedDevice::B7O;
	if( strcmp( s, "B8E" ) == 0 )
		return UsartBasedDevice::B8E;
	if( strcmp( s, "B8O" ) == 0 )
		return UsartBasedDevice::B8O;
	return UsartBasedDevice::B8N;
}

bool MarvieXmlConfigParsers::parseVPortConfig( XMLElement* node, VPortConf* conf )
{
	const char* v;
	if( node->QueryStringAttribute( "frameFormat", &v ) == XML_SUCCESS )
		conf->format = toFrameFormat( v );
	else
		conf->format = UsartBasedDevice::B8N;
	conf->baudrate = node->IntAttribute( "baudrate", 115200 );

	return true;
}

bool MarvieXmlConfigParsers::parseVPortOverIp( XMLElement* node, VPortOverIpConf* conf )
{
	const char* v;
	if( node->QueryStringAttribute( "ip", &v ) == XML_SUCCESS )
	{
		size_t len = strlen( v );
		if( len > 15 )
			return false;
		memcpy( conf->ip, v, len );
		conf->ip[len] = 0;

		int port;
		if( node->QueryIntAttribute( "port", &port ) == XML_NO_ATTRIBUTE )
			return false;
		conf->port = ( uint16_t )port;
	}
	else
		return false;

	return true;
}

bool MarvieXmlConfigParsers::parseVPortOverIpGroup( XMLElement* node, std::vector< VPortOverIpConf >* vPortOverIpList )
{
	while( node )
	{
		VPortOverIpConf conf;
		if( parseVPortOverIp( node, &conf ) )
			vPortOverIpList->push_back( conf );
		else
			return false;
		node = node->NextSiblingElement( "vPortOverIp" );
	}

	return true;
}

bool MarvieXmlConfigParsers::parseGsmModemConfig( XMLElement* node, GsmModemConf* conf )
{
	conf->pinCode = node->IntAttribute( "pinCode", 0 );
	const char* v;
	if( node->QueryStringAttribute( "apn", &v ) == XML_NO_ATTRIBUTE )
		return false;
	int len = strlen( v );
	if( len > 20 )
		return false;
	memcpy( conf->apn, v, len );
	conf->apn[len] = 0;

	return true;
}

bool MarvieXmlConfigParsers::parseModbusRtuSlaveConfig( XMLElement* node, ModbusRtuSlaveConf* conf )
{
	const char* v;
	if( node->QueryStringAttribute( "frameFormat", &v ) == XML_SUCCESS )
		conf->format = toFrameFormat( v );
	else
		conf->format = UsartBasedDevice::B8N;
	conf->baudrate = node->IntAttribute( "baudrate", 115200 );
	int addr = node->IntAttribute( "address", -1 );
	if( addr == -1 )
		return false;
	conf->address = ( uint32_t )addr;

	return true;
}

bool MarvieXmlConfigParsers::parseModbusAsciiSlaveConfig( XMLElement* node, ModbusAsciiSlaveConf* conf )
{
	const char* v;
	if( node->QueryStringAttribute( "frameFormat", &v ) == XML_SUCCESS )
		conf->format = toFrameFormat( v );
	else
		conf->format = UsartBasedDevice::B8N;
	conf->baudrate = node->IntAttribute( "baudrate", 115200 );
	int addr = node->IntAttribute( "address", -1 );
	if( addr == -1 )
		return false;
	conf->address = ( uint32_t )addr;

	return true;
}

bool MarvieXmlConfigParsers::parseMultiplexerConfig( XMLElement* node, MultiplexerConf* conf )
{
	char name[] = "com0";
	for( int i = 0; i < 5; ++i )
	{
		name[3] = '0' + i;
		XMLElement* c0 = node->FirstChildElement( name );
		if( !c0 )
			return false;

		const char* v;
		if( c0->QueryStringAttribute( "frameFormat", &v ) == XML_SUCCESS )
			conf->format[i] = toFrameFormat( v );
		else
			conf->format[i] = UsartBasedDevice::B8N;
		conf->baudrate[i] = c0->IntAttribute( "baudrate", 115200 );
	}

	return true;
}

MarvieXmlConfigParsers::ComPortConf** MarvieXmlConfigParsers::parseComPortsConfig( XMLElement* comPortsConfigNode, const std::list< std::list< ComPortAssignment > >& comPortAssignments )
{
	if( !comPortsConfigNode )
		return nullptr;
	ComPortConf** comPortsConfig = new ComPortConf*[comPortAssignments.size()];
	for( uint i = 0; i < comPortAssignments.size(); ++i )
		comPortsConfig[i] = nullptr;
	auto clear = [&]()
	{
		for( uint i = 0; i < comPortAssignments.size(); ++i )
			delete comPortsConfig[i];
		delete comPortsConfig;
	};
	uint comNum = 0;
	XMLElement* comElement = comPortsConfigNode->FirstChildElement();
	for( auto& iCom : comPortAssignments )
	{
		char comName[] = "com0";
		comName[3] = '0' + comNum;
		if( strcmp( comElement->Name(), comName ) )
		{
			clear();
			return nullptr;
		}
		XMLElement* confNode = comElement->FirstChildElement();
		bool parseResult = false;
		for( auto iAssignment : iCom )
		{
			if( iAssignment == ComPortAssignment::VPort )
			{
				if( strcmp( confNode->Name(), "vPort" ) )
					continue;
				comPortsConfig[comNum] = new VPortConf;
				parseResult = parseVPortConfig( confNode, static_cast< VPortConf* >( comPortsConfig[comNum] ) );
				break;
			}
			else if( iAssignment == ComPortAssignment::GsmModem )
			{
				if( strcmp( confNode->Name(), "gsmModem" ) )
					continue;
				comPortsConfig[comNum] = new GsmModemConf;
				parseResult = parseGsmModemConfig( confNode, static_cast< GsmModemConf* >( comPortsConfig[comNum] ) );
				break;
			}
			else if( iAssignment == ComPortAssignment::ModbusRtuSlave )
			{
				if( strcmp( confNode->Name(), "modbusRtuSlave" ) )
					continue;
				comPortsConfig[comNum] = new ModbusRtuSlaveConf;
				parseResult = parseModbusRtuSlaveConfig( confNode, static_cast< ModbusRtuSlaveConf* >( comPortsConfig[comNum] ) );
				break;
			}
			else if( iAssignment == ComPortAssignment::ModbusAsciiSlave )
			{
				if( strcmp( confNode->Name(), "modbusAsciiSlave" ) )
					continue;
				comPortsConfig[comNum] = new ModbusAsciiSlaveConf;
				parseResult = parseModbusAsciiSlaveConfig( confNode, static_cast< ModbusAsciiSlaveConf* >( comPortsConfig[comNum] ) );
				break;
			}
			else if( iAssignment == ComPortAssignment::Multiplexer )
			{
				if( strcmp( confNode->Name(), "multiplexer" ) )
					continue;
				comPortsConfig[comNum] = new MultiplexerConf;
				parseResult = parseMultiplexerConfig( confNode, static_cast< MultiplexerConf* >( comPortsConfig[comNum] ) );
				break;
			}
		}

		if( !parseResult )
		{
			clear();
			return nullptr;
		}
		++comNum;
		comElement = comElement->NextSiblingElement();
	}

	return comPortsConfig;
}

bool MarvieXmlConfigParsers::parseNetworkConfig( XMLElement* networkConfigNode, NetworkConf* conf )
{
	if( !networkConfigNode )
		return false;
	int port;
	XMLElement* c0 = networkConfigNode->FirstChildElement( "ethernet" );
	if( !c0 )
		return false;
	XMLElement* c1 = c0->FirstChildElement( "dhcp" );
	if( !c1 )
		return false;
	conf->ethConf.dhcpEnable = strcmp( c1->GetText(), "enable" ) == 0;
	c1 = c0->FirstChildElement( "ip" );
	if( !c1 )
		return false;
	conf->ethConf.ip = IpAddress( c1->GetText() );
	c1 = c0->FirstChildElement( "netmask" );
	if( !c1 )
		return false;
	conf->ethConf.netmask = *( uint32_t* )IpAddress( c1->GetText() ).addr;
	c1 = c0->FirstChildElement( "gateway" );
	if( !c1 )
		return false;
	conf->ethConf.gateway = IpAddress( c1->GetText() );
	c0 = networkConfigNode->FirstChildElement( "modbusRtuServer" );
	if( c0 )
	{
		if( c0->QueryIntAttribute( "port", &port ) == XML_NO_ATTRIBUTE )
			return false;
		conf->modbusRtuServerPort = ( uint16_t )port;
	}
	else
		conf->modbusRtuServerPort = 0;
	c0 = networkConfigNode->FirstChildElement( "modbusTcpServer" );
	if( c0 )
	{
		if( c0->QueryIntAttribute( "port", &port ) == XML_NO_ATTRIBUTE )
			return false;
		conf->modbusTcpServerPort = ( uint16_t )port;
	}
	else
		conf->modbusTcpServerPort = 0;
	c0 = networkConfigNode->FirstChildElement( "modbusAsciiServer" );
	if( c0 )
	{
		if( c0->QueryIntAttribute( "port", &port ) == XML_NO_ATTRIBUTE )
			return false;
		conf->modbusAsciiServerPort = ( uint16_t )port;
	}
	else
		conf->modbusAsciiServerPort = 0;

	c0 = networkConfigNode->FirstChildElement( "vPortOverIp" );
	if( c0 )
		return parseVPortOverIpGroup( c0, &conf->vPortOverIpList );

	return true;
}

bool MarvieXmlConfigParsers::parseSensorReadingConfig( XMLElement* sensorReadingConfigNode, SensorReadingConf* conf )
{
	if( !sensorReadingConfigNode )
		return false;
	auto c0 = sensorReadingConfigNode->FirstChildElement( "rs485MinInterval" );
	if( !c0 )
		return false;
	if( c0->QueryUnsignedAttribute( "value", ( unsigned int* )&conf->rs485MinInterval ) == XML_NO_ATTRIBUTE )
		return false;

	return true;
}

bool MarvieXmlConfigParsers::parseLogConfig( XMLElement* logConfigNode, LogConf* conf )
{
	if( !logConfigNode )
	{
		conf->maxSize = 1024;
		conf->overwriting = true;
		conf->digitInputsMode = LogConf::DigitInputsMode::Disabled;
		conf->analogInputsMode = LogConf::AnalogInputsMode::Disabled;
		conf->sensorsMode = LogConf::SensorsMode::Disabled;
		conf->digitInputsPeriod = 0;
		conf->analogInputsPeriod = 0;

		return true;
	}

	if( logConfigNode->QueryUnsignedAttribute( "maxSize", ( unsigned int* )&conf->maxSize ) == XML_NO_ATTRIBUTE )
		return false;
	if( logConfigNode->QueryBoolAttribute( "overwriting", &conf->overwriting ) == XML_NO_ATTRIBUTE )
		return false;

	auto c0 = logConfigNode->FirstChildElement( "digitInputs" );
	const char* v;
	if( c0->QueryStringAttribute( "mode", &v ) != XML_SUCCESS )
		return false;
	if( strcmp( v, "byTime" ) == 0 )
	{
		conf->digitInputsMode = LogConf::DigitInputsMode::ByTime;
		if( c0->QueryUnsignedAttribute( "period", ( unsigned int* )&conf->digitInputsPeriod ) == XML_NO_ATTRIBUTE )
			return false;
	}
	else if( strcmp( v, "byChange" ) == 0 )
	{
		conf->digitInputsMode = LogConf::DigitInputsMode::ByChange;
		if( c0->QueryUnsignedAttribute( "period", ( unsigned int* )&conf->digitInputsPeriod ) == XML_NO_ATTRIBUTE )
			return false;
	}
	else if( strcmp( v, "disabled" ) == 0 )
	{
		conf->digitInputsMode = LogConf::DigitInputsMode::Disabled;
		conf->digitInputsPeriod = 0;
	}
	else
		return false;

	c0 = logConfigNode->FirstChildElement( "analogInputs" );
	if( c0->QueryStringAttribute( "mode", &v ) != XML_SUCCESS )
		return false;
	if( strcmp( v, "byTime" ) == 0 )
	{
		conf->analogInputsMode = LogConf::AnalogInputsMode::ByTime;
		if( c0->QueryUnsignedAttribute( "period", ( unsigned int* )&conf->analogInputsPeriod ) == XML_NO_ATTRIBUTE )
			return false;
	}
	else if( strcmp( v, "disabled" ) == 0 )
	{
		conf->analogInputsMode = LogConf::AnalogInputsMode::Disabled;
		conf->analogInputsPeriod = 0;
	}
	else
		return false;

	c0 = logConfigNode->FirstChildElement( "sensors" );
	if( c0->QueryStringAttribute( "mode", &v ) != XML_SUCCESS )
		return false;
	if( strcmp( v, "enabled" ) == 0 )
		conf->sensorsMode = LogConf::SensorsMode::Enabled;
	else if( strcmp( v, "disabled" ) == 0 )
		conf->sensorsMode = LogConf::SensorsMode::Disabled;
	else
		return false;

	return true;
}
