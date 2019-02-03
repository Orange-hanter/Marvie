#pragma once

#include "AbstractSensor.h"
#include "Filters/LowPassFilter.h"
#include "Filters/MedianFilter.h"
#include "Filters/MovingAvgFilter.h"

class AnalogSensor : public AbstractSRSensor
{
public:
	class Data : public SensorData
	{
		friend class AnalogSensor;
	public:
		Data()
		{
			value = 0.0f;
		}
		float value;
	};
	enum class FilterType
	{
		None,
		LowPassAlpha,
		LowPassFreq,
		MovingAvg,
		Median
	};

	explicit AnalogSensor();
	~AnalogSensor();

	inline static const char* sName() { return "AnalogSensor"; }
	const char* name() const final override;

	void setSignalChannel( uint16_t blockId, uint16_t line );
	void setFilterSettings( FilterType filterAType, float prmA, FilterType filterBType, float prmB, float dt );
	void setLinearMappingSettings( float k, float b );

	Data* readData() final override;
	Data* sensorData() final override;
	uint32_t sensorDataSize() final override;

private:
	uint16_t blockId;
	uint16_t line;
    Abstract1DFilter* filterA;
    Abstract1DFilter* filterB;
    float k, b;
	Data data;
};