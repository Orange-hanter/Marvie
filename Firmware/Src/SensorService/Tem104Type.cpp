#include "SensorService.h"
#include "Drivers/Sensors/Tem104Sensor.h"

using namespace tinyxml2;

static AbstractSensor* tem104Allocate()
{
	return new Tem104Sensor;
}

static bool tem104Tune( AbstractSensor* sensor, XMLElement* e, uint32_t defaultBaudrate )
{
	XMLElement* c0 = e->FirstChildElement( "baudrate" );
	if( c0 )
		defaultBaudrate = c0->UnsignedText( defaultBaudrate );
	static_cast< Tem104Sensor* >( sensor )->setBaudrate( defaultBaudrate );

	c0 = e->FirstChildElement( "address" );
	if( !c0 )
		return false;
	static_cast< Tem104Sensor* >( sensor )->setAddress( c0->IntText( 0 ) );

	return true;
}

static SensorService::Node* tem104Type = []()
{
	static SensorService::Node node;
	node.value.type = AbstractSensor::Type::BR;
	node.value.name = Tem104Sensor::Name;
	node.value.allocator = tem104Allocate;
	node.value.tuner = tem104Tune;
	SensorService::registerSensorType( &node );
	return &node;
}();