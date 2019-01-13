#include "SensorService.h"
#include "Drivers/Sensors/Tem104USensor.h"

using namespace tinyxml2;

static AbstractSensor* tem104UAllocate()
{
	return new Tem104USensor;
}

static bool tem104UTune( AbstractSensor* sensor, XMLElement* e, uint32_t defaultBaudrate )
{
	XMLElement* c0 = e->FirstChildElement( "baudrate" );
	if( c0 )
		defaultBaudrate = c0->UnsignedText( defaultBaudrate );
	static_cast< Tem104USensor* >( sensor )->setBaudrate( defaultBaudrate );

	c0 = e->FirstChildElement( "address" );
	if( !c0 )
		return false;
	static_cast< Tem104USensor* >( sensor )->setAddress( c0->IntText( 0 ) );

	return true;
}

static SensorService::Node* tem104UType = []()
{
	static SensorService::Node node;
	node.value.type = AbstractSensor::Type::BR;
	node.value.name = Tem104USensor::Name;
	node.value.allocator = tem104UAllocate;
	node.value.tuner = tem104UTune;
	SensorService::registerSensorType( &node );
	return &node;
}( );