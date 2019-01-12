#include "GeneralSRSensor.h"
#include "Core/DateTimeService.h"

GeneralSRSensor::GeneralSRSensor()
{
	blockId = 0;
	line = 0;
	filterA = filterB = nullptr;
	k = 1.0f;
	b = 0.0f;
}

GeneralSRSensor::~GeneralSRSensor()
{
	delete filterA;
	delete filterB;
}

const char* GeneralSRSensor::name() const
{
	return sName();
}

void GeneralSRSensor::setSignalChannel( uint16_t blockId, uint16_t line )
{
	this->blockId = blockId;
	this->line = line;
}

void GeneralSRSensor::setFilterSettings( FilterType filterAType, float prmA, FilterType filterBType, float prmB, float dt )
{
	uint32_t prm;
	switch( filterAType )
	{
	case GeneralSRSensor::FilterType::None:
		delete filterA;
		filterA = nullptr;
		break;
	case GeneralSRSensor::FilterType::LowPassAlpha:
		filterA = new LowPassFilter;
		static_cast< LowPassFilter* >( filterA )->setAlpha( prmA );
		break;
	case GeneralSRSensor::FilterType::LowPassFreq:
		filterA = new LowPassFilter;
		static_cast< LowPassFilter* >( filterA )->setFreq( dt, prmA );
		break;
	case GeneralSRSensor::FilterType::MovingAvg:
		prm = ( uint32_t )prmA;
		if( prm < 2 )
			prm = 2;
		filterA = new MovingAvgFilter( prm );
		break;
	case GeneralSRSensor::FilterType::Median:
		prm = ( uint32_t )prmA;
		if( prm < 3 )
			prm = 3;
		else if( ( prm & 1 ) == 0 )
			++prm;
		filterA = new MedianFilter( prm );
		break;
	default:
		break;
	}
	switch( filterBType )
	{
	case GeneralSRSensor::FilterType::None:
		delete filterB;
		filterB = nullptr;
		break;
	case GeneralSRSensor::FilterType::LowPassAlpha:
		filterB = new LowPassFilter;
		static_cast< LowPassFilter* >( filterB )->setAlpha( prmB );
		break;
	case GeneralSRSensor::FilterType::LowPassFreq:
		filterB = new LowPassFilter;
		static_cast< LowPassFilter* >( filterB )->setFreq( dt, prmB );
		break;
	case GeneralSRSensor::FilterType::MovingAvg:
		prm = ( uint32_t )prmB;
		if( prm < 2 )
			prm = 2;
		filterB = new MovingAvgFilter( prm );
		break;
	case GeneralSRSensor::FilterType::Median:
		prm = ( uint32_t )prmB;
		if( prm < 3 )
			prm = 3;
		else if( ( prm & 1 ) == 0 )
			++prm;
		filterB = new MedianFilter( prm );
		break;
	default:
		break;
	}

	if( filterA )
		filterA->reset( signalProvider->analogSignal( blockId, line ) );
	if( filterB )
		filterB->reset( signalProvider->analogSignal( blockId, line ) );
}

void GeneralSRSensor::setLinearMappingSettings( float k, float b )
{
	this->k = k;
	this->b = b;
}

GeneralSRSensor::Data* GeneralSRSensor::readData()
{
	data.t = DateTimeService::currentDateTime();
	float value = signalProvider->analogSignal( blockId, line );
	if( filterA )
		value = filterA->update( value );
	if( filterB )
		value = filterB->update( value );
	data.value = k * value + b;

	return &data;
}

GeneralSRSensor::Data* GeneralSRSensor::sensorData()
{
	return &data;
}

uint32_t GeneralSRSensor::sensorDataSize()
{
	return sizeof( Data ) - sizeof( SensorData );
}