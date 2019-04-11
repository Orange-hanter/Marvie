#include "SensorService.h"
#include "Drivers/Sensors/Tem104M1Sensor.h"

using namespace tinyxml2;

static AbstractSensor* tem104M1Allocate()
{
	return new Tem104M1Sensor;
}

static bool tem104M1Tune( AbstractSensor* sensor, XMLElement* e, uint32_t defaultBaudrate )
{
	XMLElement* c0 = e->FirstChildElement( "baudrate" );
	if( c0 )
		defaultBaudrate = c0->UnsignedText( defaultBaudrate );
	static_cast< Tem104M1Sensor* >( sensor )->setBaudrate( defaultBaudrate );

	c0 = e->FirstChildElement( "address" );
	if( !c0 )
		return false;
	static_cast< Tem104M1Sensor* >( sensor )->setAddress( c0->IntText( 0 ) );

	return true;
}

static SensorService::Node* tem104M1Type = []()
{
	static SensorService::Node node;
	node.value.type = AbstractSensor::Type::BR;
	node.value.name = Tem104M1Sensor::Name;
	node.value.allocator = tem104M1Allocate;
	node.value.tuner = tem104M1Tune;
	SensorService::registerSensorType( &node );
	return &node;
}( );