#include "SensorService.h"
#include "Drivers/Sensors/TestSRSensorA.h"

using namespace tinyxml2;

static AbstractSensor* testSRSensorAAllocate()
{
	return new TestSRSensorA;
}

static bool testSRSensorATune( AbstractSensor* sensor, XMLElement* e, uint32_t defaultValue )
{
	auto c0 = e->FirstChildElement( "blockID" );
	int blockId;
	if( !c0 || c0->QueryIntText( &blockId ) != XML_SUCCESS )
		return false;

	c0 = e->FirstChildElement( "channel" );
	int line;
	if( !c0 || c0->QueryIntText( &line ) != XML_SUCCESS )
		return false;

	static_cast< TestSRSensorA* >( sensor )->setSignalChannel( blockId, line );

	return true;
}

static SensorService::Node* simpleSensorType = []()
{
	static SensorService::Node node;
	node.value.type = AbstractSensor::Type::BR;
	node.value.name = TestSRSensorA::sName();
	node.value.allocator = testSRSensorAAllocate;
	node.value.tuner = testSRSensorATune;
	SensorService::registerSensorType( &node );
	return &node;
}();