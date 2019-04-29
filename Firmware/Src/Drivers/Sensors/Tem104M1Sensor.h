#pragma once

#include "AbstractSensor.h"
#include "Drivers/Interfaces/UsartBasedDevice.h"

class Tem104M1Sensor : public AbstractBRSensor
{
public:
	static const char Name[];
	class Data : public SensorData
	{
		friend class Tem104M1Sensor;
	public:
		Data()
		{
			constexpr int size = ( sizeof( Data ) - sizeof( SensorData ) ) / 4;
			for( int i = 0; i < size; ++i )
				reinterpret_cast< uint32_t* >( &instant )[i] = 0;
		}
		struct InstantValues
		{
			float rshv;		    // ������� �������� ��������� �������
			float rshm;		    // ������� �������� ��������� �������
			float tmp[2];	    // ������� �������� ����������� �� �������
			float prs[2];	    // ������� �������� �������� �� �������
		} instant;
		struct IntegratedValues
		{
			double intV;	    // ���������� ������ �� ������
			double intM;	    // ���������� ����� �� ������
			double intQ;	    // ���������� ������� �� ������
			uint32_t T_Rab;     // ����� ����� ������
			uint32_t T_Nar;
			uint32_t T_offline;	// ����� ��������� ��� ������
			uint32_t T_Fail;	// ����� ���������� � �������
			uint32_t T_Dt;		// ����� � ������ dT < dTmin
			uint32_t T_Gmax;	// ����� ���������� � ������ G > Gmax 
			uint32_t T_Gmin;	// ����� ���������� � ������ G < Gmin
			uint32_t reserved;
			uint16_t tmp[2];	// �����������
			uint8_t prs[2];		// ��������
			uint8_t TekErr;		// ������� ������ 1
			uint8_t TehErr;		// ������� ������ 2
		} integrated;
	};

	explicit Tem104M1Sensor();
	virtual ~Tem104M1Sensor();

	const char* name() const final override;
	void setAddress( uint8_t address );
	void setBaudrate( uint32_t baudrate );

	Data* readData() final override;
	Data* sensorData() final override;
	uint32_t sensorDataSize() final override;

private:
	SensorData::Error waitResponse( uint32_t size );
	void parseTimerResponse();
	void parseRamResponse();
	template < typename T > uint8_t checksum( T, T ) const;

private:
	uint8_t address;
	uint32_t baudrate;
	Data data;
};
