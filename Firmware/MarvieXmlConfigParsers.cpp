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
		strncpy( conf->ip, v, len );
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

bool MarvieXmlConfigParsers::parseVPortOverIpGroup( XMLElement* node, std::list< VPortOverIpConf >* vPortOverIpList )
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
	if( node->QueryStringAttribute( "vpn", &v ) == XML_NO_ATTRIBUTE )
		return false;
	int len = strlen( v );
	if( len > 20 )
		return false;
	strncpy( conf->vpn, v, len );
	conf->vpn[len] = 0;

	int port;
	XMLElement* c0 = node->FirstChildElement( "modbusRtuServer" );
	if( c0 )
	{
		if( c0->QueryIntAttribute( "port", &port ) == XML_NO_ATTRIBUTE )
			return false;
		conf->modbusRtuServerPort = ( uint16_t )port;
	}
	else
		conf->modbusRtuServerPort = 0;
	c0 = node->FirstChildElement( "modbusTcpServer" );
	if( c0 )
	{
		if( c0->QueryIntAttribute( "port", &port ) == XML_NO_ATTRIBUTE )
			return false;
		conf->modbusTcpServerPort = ( uint16_t )port;
	}
	else
		conf->modbusTcpServerPort = 0;
	c0 = node->FirstChildElement( "modbusAsciiServer" );
	if( c0 )
	{
		if( c0->QueryIntAttribute( "port", &port ) == XML_NO_ATTRIBUTE )
			return false;
		conf->modbusAsciiServerPort = ( uint16_t )port;
	}
	else
		conf->modbusAsciiServerPort = 0;

	c0 = node->FirstChildElement( "vPortOverIp" );
	if( c0 )
		return parseVPortOverIpGroup( c0, &conf->vPortOverIpList );

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

bool MarvieXmlConfigParsers::parseEthernetConfig( XMLElement* ethernetConfigNode, EthernetConf* conf )
{
	int port;
	XMLElement* c0 = ethernetConfigNode->FirstChildElement( "modbusRtuServer" );
	if( c0 )
	{
		if( c0->QueryIntAttribute( "port", &port ) == XML_NO_ATTRIBUTE )
			return false;
		conf->modbusRtuServerPort = ( uint16_t )port;
	}
	else
		conf->modbusRtuServerPort = 0;
	c0 = ethernetConfigNode->FirstChildElement( "modbusTcpServer" );
	if( c0 )
	{
		if( c0->QueryIntAttribute( "port", &port ) == XML_NO_ATTRIBUTE )
			return false;
		conf->modbusTcpServerPort = ( uint16_t )port;
	}
	else
		conf->modbusTcpServerPort = 0;
	c0 = ethernetConfigNode->FirstChildElement( "modbusAsciiServer" );
	if( c0 )
	{
		if( c0->QueryIntAttribute( "port", &port ) == XML_NO_ATTRIBUTE )
			return false;
		conf->modbusAsciiServerPort = ( uint16_t )port;
	}
	else
		conf->modbusAsciiServerPort = 0;

	c0 = ethernetConfigNode->FirstChildElement( "vPortOverIp" );
	if( c0 )
		return parseVPortOverIpGroup( c0, &conf->vPortOverIpList );

	return true;
}