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
				reinterpret_cast< float* >( G )[i] = 0;
		}

		float G[2]; // m^3/h
		float P[2]; // KW
		float Q[2]; // MW*h
		float V[2]; // m^3
		float M[2]; // tn
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