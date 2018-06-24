#include "SensorService.h"
#include "Drivers/Sensors/CE301Sensor.h"

using namespace tinyxml2;

static AbstractBRSensor* ce301Allocate()
{
	return new CE301Sensor;
}

static bool ce301Tune( AbstractBRSensor* sensor, XMLElement* e )
{
	XMLElement* c0 = e->FirstChildElement( "address" );
	if( !c0 )
		return false;
	static_cast< CE301Sensor* >( sensor )->setAddress( c0->IntText( 0 ) );
	return true;
}

static BRSensorService::Node* ce301Type = []()
{
	static BRSensorService::Node node;
	node.value.name = CE301Sensor::Name;
	node.value.allocator = ce301Allocate;
	node.value.tuner = ce301Tune;
	BRSensorService::registerSensorType( &node );
	return &node;
}( );