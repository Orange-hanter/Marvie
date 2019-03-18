#pragma once

#include "AbstractSensor.h"
#include "Drivers/Interfaces/UsartBasedDevice.h"

class Tem05MSensor : public AbstractBRSensor
{
public:
	static const char Name[];
	class Data : public SensorData
	{
		friend class Tem05MSensor;

	public:
		Data()
		{
			constexpr uint32_t size = ( sizeof( Data ) - sizeof( SensorData ) ) / sizeof( float );
			for( uint32_t i = 0; i < size; ++i )
				reinterpret_cast< float* >( ch )[i] = 0;
		}

		struct Channel
		{
			float Gmax; // m^3/h
			float Gmin; // m^3/h
			float G;    // m^3/h
			float P;    // KW
			float Q;    // MW*h
			float V;    // m^3
			float M;    // tn
			float t;    // Celsius
		} ch[2];
		float t3;       // programmable temperature
		float tCW;      // cold water temperature
		uint32_t workingTime;
		uint8_t scheme;
		uint8_t pipeDiameter[2];
		uint8_t error;
	};

	explicit Tem05MSensor();
	virtual ~Tem05MSensor();

	const char* name() const final override;
	void setBaudrate( uint32_t baudrate );

	Data* readData() final override;
	Data* sensorData() final override;
	uint32_t sensorDataSize() final override;

private:
	bool parseData();

private:
	uint32_t baudrate;
	Data data;
};