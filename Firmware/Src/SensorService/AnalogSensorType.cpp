#include "Drivers/Sensors/AnalogSensor.h"
#include "MarviePlatform.h"
#include "SensorService.h"
#include <string.h>

using namespace tinyxml2;

static AbstractSensor* generalSRSensorAllocate()
{
	return new AnalogSensor;
}

AnalogSensor::FilterType filterType( const char* filterName )
{
	if( strcmp( filterName, "LowPassAlpha" ) == 0 )
		return AnalogSensor::FilterType::LowPassAlpha;
	if( strcmp( filterName, "LowPassFreq" ) == 0 )
		return AnalogSensor::FilterType::LowPassFreq;
	if( strcmp( filterName, "MovingAvg" ) == 0 )
		return AnalogSensor::FilterType::MovingAvg;
	if( strcmp( filterName, "Median" ) == 0 )
		return AnalogSensor::FilterType::Median;

	return AnalogSensor::FilterType::None;
}

static bool generalSRSensorTune( AbstractSensor* sensor, XMLElement* e, uint32_t defaultValue )
{
	auto c0 = e->FirstChildElement( "blockID" );
	int blockId;
	if( !c0 || c0->QueryIntText( &blockId ) != XML_SUCCESS )
		return false;

	c0 = e->FirstChildElement( "channel" );
	int line;
	if( !c0 || c0->QueryIntText( &line ) != XML_SUCCESS )
		return false;

	c0 = e->FirstChildElement( "filterA" );
	if( !c0 )
		return false;
	const char* filterAName = c0->GetText();

	c0 = e->FirstChildElement( "prmA" );
	float prmA;
	if( !c0 || c0->QueryFloatText( &prmA ) != XML_SUCCESS )
		return false;

	c0 = e->FirstChildElement( "filterB" );
	if( !c0 )
		return false;
	const char* filterBName = c0->GetText();

	c0 = e->FirstChildElement( "prmB" );
	float prmB;
	if( !c0 || c0->QueryFloatText( &prmB ) != XML_SUCCESS )
		return false;

	c0 = e->FirstChildElement( "k" );
	float k;
	if( !c0 || c0->QueryFloatText( &k ) != XML_SUCCESS )
		return false;

	c0 = e->FirstChildElement( "b" );
	float b;
	if( !c0 || c0->QueryFloatText( &b ) != XML_SUCCESS )
		return false;

	static_cast< AnalogSensor* >( sensor )->setSignalChannel( blockId, line );
	static_cast< AnalogSensor* >( sensor )->setFilterSettings( filterType( filterAName ), prmA, filterType( filterBName ), prmB, MarviePlatform::srSensorUpdatePeriodMs / 1000.0f );
	static_cast< AnalogSensor* >( sensor )->setLinearMappingSettings( k, b );

	return true;
}

static SensorService::Node* generalSRSensorType = []()
{
	static SensorService::Node node;
	node.value.type = AbstractSensor::Type::SR;
	node.value.name = AnalogSensor::sName();
	node.value.allocator = generalSRSensorAllocate;
	node.value.tuner = generalSRSensorTune;
	SensorService::registerSensorType( &node );
	return &node;
}();