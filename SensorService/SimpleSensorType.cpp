#include "SensorService.h"
#include "Drivers/Sensors/SimpleSensor.h"

using namespace tinyxml2;

static AbstractBRSensor* simpleSensorAllocate()
{
	return new SimpleSensor;
}

static bool simpleSensorTune( AbstractBRSensor* sensor, XMLElement* e )
{
	XMLElement* c0 = e->FirstChildElement( "address" );
	if( !c0 )
		return false;
	static_cast< SimpleSensor* >( sensor )->setAddress( c0->IntText( 0 ) );
	return true;
}

static BRSensorService::Node* simpleSensorType = []()
{
	static BRSensorService::Node node;
	node.value.name = SimpleSensor::sName();
	node.value.allocator = simpleSensorAllocate;
	node.value.tuner = simpleSensorTune;
	BRSensorService::registerSensorType( &node );
	return &node;
}();