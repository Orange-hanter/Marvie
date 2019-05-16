#pragma once

#include "AbstractSensor.h"
#include "Drivers/Interfaces/UsartBasedDevice.h"

class Tem104M2Sensor : public AbstractBRSensor
{
public:
	static const char Name[];
	class Data : public SensorData
	{
		friend class Tem104M2Sensor;
	public:
		Data()
		{
			constexpr int size = ( sizeof( Data ) - sizeof( SensorData ) ) / 4;
			for( int i = 0; i < size; ++i )
				reinterpret_cast< uint32_t* >( &instant )[i] = 0;
		}
		struct InstantValues
		{
			struct Ch
			{
				float tmp[4];	    // current values of temperatures, t C
				float prs[4];	    // current values of pressure		MPa
				float ro[4];		// current values of density heat currier
				float hent[4];		// current values of enthalpy
				float rshv[4];		// current values of volume rate	m^3/h
				float rshm[4];		// current values of mass rate		Tonn/h
				float pwr[4];		// current values of power values	Gigacaloria/h
				
			} ch[4];
			
		} instant;
		struct IntegratedValues
		{
			uint32_t utc;
			uint32_t T_Rab;     // total work time
			uint32_t T_offline;	// time since last shutdown
			uint32_t reserved;
			struct Ch
			{
				double intV;	    // counted volume
				double intM;	    // counted mass
				double intQ;	    // counted energy
				double intQerr;		// counted energy with error
				uint32_t T_Nar;		// system runtime without errors
				uint32_t T_Max;		// system runtime with error G > Gmax 
				uint32_t T_Min;		// system runtime with error G < Gmin
				uint32_t T_Dt;		// system runtime with error dT < dTmin
				uint32_t T_tn;		//
				uint32_t T_rev;		// system runtime with reverce in system
				uint32_t T_pt;		//
				uint8_t TekErr;		// byte error block 1
				uint8_t reserved;
				uint16_t TehErr;	// byte error block 2
				uint16_t tmp[3];	// temperature by chanell
				uint16_t prs[3];	// pressure by chanell
				uint32_t reserved2;
			} ch[4];
		} integrated;
		
	};

	explicit Tem104M2Sensor();
	virtual ~Tem104M2Sensor();

	const char* name() const final override;
	void setAddress( uint8_t address );
	void setBaudrate( uint32_t baudrate );

	Data* readData() final override;
	Data* sensorData() final override;
	uint32_t sensorDataSize() final override;

private:
	SensorData::Error waitResponse( uint32_t size );
	void parseTimerResponsePart1();
	void parseTimerResponsePart11();
	void parseTimerResponsePart2();
	
	void parseTimerResponsePart22();
	
	void parseRamResponse(uint16_t chanel);
	template < typename T > uint8_t checksum( T, T ) const;

private:
	uint8_t address;
	uint32_t baudrate;
	Data data;
};
