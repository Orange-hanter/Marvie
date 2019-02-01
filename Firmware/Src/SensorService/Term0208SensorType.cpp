#include "SensorService.h"
#include "Drivers/Sensors/Term0208Sensor.h"

using namespace tinyxml2;

static AbstractSensor* term0208Allocate()
{
	return new Term0208Sensor;
}

static bool term0208Tune( AbstractSensor* sensor, XMLElement* e, uint32_t defaultBaudrate )
{
	XMLElement* c0 = e->FirstChildElement( "baudrate" );
	if( c0 )
		defaultBaudrate = c0->UnsignedText( defaultBaudrate );
	static_cast< Term0208Sensor* >( sensor )->setBaudrate( defaultBaudrate );

	c0 = e->FirstChildElement( "address" );
	if( !c0 )
		return false;
	static_cast< Term0208Sensor* >( sensor )->setAddress( ( uint8_t )c0->IntText( 0 ) );

	return true;
}

static SensorService::Node* term0208Type = []()
{
	static SensorService::Node node;
	node.value.type = AbstractSensor::Type::BR;
	node.value.name = Term0208Sensor::Name;
	node.value.allocator = term0208Allocate;
	node.value.tuner = term0208Tune;
	SensorService::registerSensorType( &node );
	return &node;
}();