#include "SensorService.h"
#include "Drivers/Sensors/SimpleSensor.h"

using namespace tinyxml2;

static AbstractSensor* simpleSensorAllocate()
{
	return new SimpleSensor;
}

static bool simpleSensorTune( AbstractSensor* sensor, XMLElement* e, uint32_t defaultBaudrate )
{
	/*XMLElement* c0 = e->FirstChildElement( "address" );
	if( !c0 )
		return false;
	static_cast< SimpleSensor* >( sensor )->setAddress( c0->IntText( 0 ) );*/
	return true;
}

static SensorService::Node* simpleSensorType = []()
{
	static SensorService::Node node;
	node.value.type = AbstractSensor::Type::BR;
	node.value.name = SimpleSensor::sName();
	node.value.allocator = simpleSensorAllocate;
	node.value.tuner = simpleSensorTune;
	SensorService::registerSensorType( &node );
	return &node;
}();