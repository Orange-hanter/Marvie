#include "SensorService.h"
#include "Drivers/Sensors/Term0204Sensor.h"

using namespace tinyxml2;

static AbstractSensor* term0204Allocate()
{
	return new TERM0204Sensor;
}

static bool term0204Tune( AbstractSensor* sensor, XMLElement* e, uint32_t defaultBaudrate )
{
	XMLElement* c0 = e->FirstChildElement( "baudrate" );
	if( c0 )
		defaultBaudrate = c0->UnsignedText( defaultBaudrate );
	static_cast< TERM0204Sensor* >( sensor )->setBaudrate( defaultBaudrate );

	c0 = e->FirstChildElement( "address" );
	if( !c0 )
		return false;
	static_cast< TERM0204Sensor* >( sensor )->setAddress( c0->IntText( 0 ) );

	return true;
}

static SensorService::Node* term0204Type = []()
{
	static SensorService::Node node;
	node.value.type = AbstractSensor::Type::BR;
	node.value.name = TERM0204Sensor::Name;
	node.value.allocator = term0204Allocate;
	node.value.tuner = term0204Tune;
	SensorService::registerSensorType( &node );
	return &node;
}();