#include "SensorService.h"
#include "Drivers/Sensors/SKM2Sensor.h"

using namespace tinyxml2;

static AbstractSensor* skm2Allocate()
{
	return new SKM2Sensor;
}

static bool skm2Tune( AbstractSensor* sensor, XMLElement* e, uint32_t defaultBaudrate )
{
	XMLElement* c0 = e->FirstChildElement( "baudrate" );
	if( c0 )
		defaultBaudrate = c0->UnsignedText( defaultBaudrate );
	static_cast< SKM2Sensor* >( sensor )->setBaudrate( defaultBaudrate );

	c0 = e->FirstChildElement( "address" );
	if( !c0 )
		return false;
	static_cast< SKM2Sensor* >( sensor )->setAddress( c0->IntText( 0 ) );

	return true;
}

static SensorService::Node* skm2Type = []()
{
	static SensorService::Node node;
	node.value.type = AbstractSensor::Type::BR;
	node.value.name = SKM2Sensor::Name;
	node.value.allocator = skm2Allocate;
	node.value.tuner = skm2Tune;
	SensorService::registerSensorType( &node );
	return &node;
}();