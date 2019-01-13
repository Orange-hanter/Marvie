#pragma once

#include "AbstractSensor.h"
#include "Drivers/Interfaces/UsartBasedDevice.h"

class TERM0204Sensor : public AbstractBRSensor
{
public:
	static const char Name[];
	class Data : public SensorData
	{
		friend class TERM0204Sensor;
	public:
		Data()
		{
			constexpr size_t size = ( sizeof( Data ) - sizeof( SensorData ) ) / 4;
			for( size_t i = 0; i < size; ++i )
				reinterpret_cast< uint32_t* >( &instantValues )[i] = 0;
		}
		struct InstantValues
		{
			struct Channel
			{
				float G;		 // Tn/sec
				float P;		 // Wt
				float tmpSupply; // C
				float tmpReturn; // C
			} ch[2];
			float pSupply;		 // MPa
			float pReturn;		 // MPa
			float vAddition;	 // m3/sec
			uint16_t error;
			uint16_t reserved;
		} instantValues;
		struct IntegratedValues
		{
			struct Channel
			{
				float vSumm;	 // m3
				float mSumm;	 // Tn
				float QSumm;	 // GJoules
			} ch[2];
			float vSummAddition; // m3
		} integratedValues;
	};

	explicit TERM0204Sensor();
	virtual ~TERM0204Sensor();

	const char* name() const final override;
	void setAddress( uint16_t address );
	void setBaudrate( uint32_t baudrate );

	Data* readData() final override;
	Data* sensorData() final override;
	uint32_t sensorDataSize() final override;

private:
	SensorData::Error waitResponse( uint32_t size );
	void parseTimerResponse();
	template < typename T > uint8_t checksum( T, T ) const;
	inline uint8_t toByte( char cHigh, char cLow ) const;
	inline uint8_t toByte( ByteRingIterator& i ) const;
	inline uint8_t toByte( const uint8_t* ) const;
	float b3ToFloat( uint8_t b0, uint8_t b1, uint8_t b2 );
	float b4ToFloat( uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3 );
	inline float b3ToFloat( const uint8_t* p );
	inline float b4ToFloat( const uint8_t* p );
private:
	uint16_t address;
	uint32_t baudrate;
	Data data;
};
