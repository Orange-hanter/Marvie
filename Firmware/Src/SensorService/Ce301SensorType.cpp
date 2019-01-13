#include "SensorService.h"
#include "Drivers/Sensors/CE301Sensor.h"

using namespace tinyxml2;

static AbstractSensor* ce301Allocate()
{
	return new CE301Sensor;
}

static bool ce301Tune( AbstractSensor* sensor, XMLElement* e, uint32_t defaultBaudrate )
{
	XMLElement* c0 = e->FirstChildElement( "baudrate" );
	if( c0 )
		defaultBaudrate = c0->UnsignedText( defaultBaudrate );
	static_cast< CE301Sensor* >( sensor )->setBaudrate( defaultBaudrate );

	c0 = e->FirstChildElement( "address" );
	if( !c0 )
		return false;
	static_cast< CE301Sensor* >( sensor )->setAddress( c0->IntText( 0 ) );

	return true;
}

static SensorService::Node* ce301Type = []()
{
	static SensorService::Node node;
	node.value.type = AbstractSensor::Type::BR;
	node.value.name = CE301Sensor::Name;
	node.value.allocator = ce301Allocate;
	node.value.tuner = ce301Tune;
	SensorService::registerSensorType( &node );
	return &node;
}();