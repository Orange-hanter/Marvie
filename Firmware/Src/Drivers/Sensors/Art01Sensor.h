#pragma once

#include "AbstractSensor.h"
#include "Drivers/Interfaces/UsartBasedDevice.h"

class Art01Sensor : public AbstractBRSensor
{
public:
	static const char Name[];
	class Data : public SensorData
	{
		friend class Art01Sensor;
	public:
		Data()
		{
			constexpr size_t size = ( sizeof( Data ) - sizeof( SensorData ) ) / 4;
			for( int i = 0; i < size; ++i )
				reinterpret_cast< uint32_t* >( &instant )[i] = 0;
		}
		struct InstantValues
		{
            int16_t tmp[4];
            uint16_t status;
			uint16_t reserved0;
			uint32_t reserved1;
		} instant;
	};

	explicit Art01Sensor();
	virtual ~Art01Sensor();

	const char* name() const final override;
	void setAddress( uint8_t address );
	void setBaudrate( uint32_t baudrate );

	Data* readData() final override;
	Data* sensorData() final override;
	uint32_t sensorDataSize() final override;

private:
	SensorData::Error waitResponse( uint32_t size );
	void parseTimerResponse();
	void parseRamResponse( uint32_t ch );
	template < typename T > uint8_t checksum( T, T ) const;

private:
	uint8_t address;
	uint32_t baudrate;
	Data data;
};
