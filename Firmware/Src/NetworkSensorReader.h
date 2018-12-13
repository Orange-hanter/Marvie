#pragma once

#include "Network/TcpSocket.h"
#include "SingleBRSensorReader.h"

class NetworkSensorReader : public SingleBRSensorReader
{
public:
	NetworkSensorReader();
	~NetworkSensorReader();

	void setSensorAddress( IpAddress addr, uint16_t port );

	bool startReading( tprio_t prio ) override;

private:
	void main() override;

private:
	TcpSocket socket;
	IpAddress addr;
	uint16_t port;
};
