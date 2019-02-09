#include "SensorService.h"
#include "Drivers/Sensors/Mr01Sensor.h"

using namespace tinyxml2;

static AbstractSensor* mr01Allocate()
{
	return new MR01Sensor;
}

static bool mr01Tune( AbstractSensor* sensor, XMLElement* e, uint32_t defaultBaudrate )
{
	XMLElement* c0 = e->FirstChildElement( "baudrate" );
	if( c0 )
		defaultBaudrate = c0->UnsignedText( defaultBaudrate );
	static_cast< MR01Sensor* >( sensor )->setBaudrate( defaultBaudrate );

	c0 = e->FirstChildElement( "address" );
	if( !c0 )
		return false;
	static_cast< MR01Sensor* >( sensor )->setAddress( c0->IntText( 0 ) );

	return true;
}

static SensorService::Node* art01Type = []()
{
	static SensorService::Node node;
	node.value.type = AbstractSensor::Type::BR;
	node.value.name = MR01Sensor::Name;
	node.value.allocator = mr01Allocate;
	node.value.tuner = mr01Tune;
	SensorService::registerSensorType( &node );
	return &node;
}();