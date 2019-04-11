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
				float tmp[4];	    // ������� �������� ����������� �� �������
				float prs[4];	    // ������� �������� �������� �� �������
				float ro[4];
				float hent[4];
				float rshv[4];		    // ������� �������� ��������� �������
				float rshm[4];		    // ������� �������� ��������� �������
				float pwr[4];
				
			} ch[4];
			
		} instant;
		struct IntegratedValues
		{
			uint32_t T_Rab;     // ����� ����� ������
			uint32_t T_offline;	// ����� ��������� ��� ������

			struct Ch
			{
				double intV;	    // ���������� ������ �� ������
				double intM;	    // ���������� ����� �� ������
				double intQ;	    // ���������� ������� �� ������
				double intQerr;
				uint32_t T_Nar;
				uint32_t T_Max;	// ����� ���������� � ������ G > Gmax 
				uint32_t T_Min;	// ����� ���������� � ������ G < Gmin
				uint32_t T_Dt;		// ����� � ������ dT < dTmin
				uint32_t T_tn;
				uint32_t T_rev;
				uint32_t T_pt;
				uint8_t TekErr;		// ������� ������ 1
				uint8_t TehErr;		// ������� ������ 2
				uint16_t tmp[3];	// �����������
				uint16_t prs[3];		// ��������
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
