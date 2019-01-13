#include "SensorService.h"
#include "Drivers/Sensors/Art01Sensor.h"

using namespace tinyxml2;

static AbstractSensor* art01Allocate()
{
	return new Art01Sensor;
}

static bool art01Tune( AbstractSensor* sensor, XMLElement* e, uint32_t defaultBaudrate )
{
	XMLElement* c0 = e->FirstChildElement( "baudrate" );
	if( c0 )
		defaultBaudrate = c0->UnsignedText( defaultBaudrate );
	static_cast< Art01Sensor* >( sensor )->setBaudrate( defaultBaudrate );

	c0 = e->FirstChildElement( "address" );
	if( !c0 )
		return false;
	static_cast< Art01Sensor* >( sensor )->setAddress( c0->IntText( 0 ) );

	return true;
}

static SensorService::Node* art01Type = []()
{
	static SensorService::Node node;
	node.value.type = AbstractSensor::Type::BR;
	node.value.name = Art01Sensor::Name;
	node.value.allocator = art01Allocate;
	node.value.tuner = art01Tune;
	SensorService::registerSensorType( &node );
	return &node;
}();