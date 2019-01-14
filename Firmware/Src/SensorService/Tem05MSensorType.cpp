#include "SensorService.h"
#include "Drivers/Sensors/Tem05MSensor.h"

using namespace tinyxml2;

static AbstractSensor* tem05MSensorAllocate()
{
	return new Tem05MSensor;
}

static bool tem05MSensorTune( AbstractSensor* sensor, XMLElement* e, uint32_t defaultBaudrate )
{
	XMLElement* c0 = e->FirstChildElement( "baudrate" );
	if( c0 )
		defaultBaudrate = c0->UnsignedText( defaultBaudrate );
	static_cast< Tem05MSensor* >( sensor )->setBaudrate( defaultBaudrate );

	return true;
}

static SensorService::Node* tem05MSensorType = []()
{
	static SensorService::Node node;
	node.value.type = AbstractSensor::Type::BR;
	node.value.name = Tem05MSensor::Name;
	node.value.allocator = tem05MSensorAllocate;
	node.value.tuner = tem05MSensorTune;
	SensorService::registerSensorType( &node );
	return &node;
}();