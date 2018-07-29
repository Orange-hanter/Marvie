#include "Network/UdpSocket.h"

class UdpStressTestServer
{
public:
	UdpStressTestServer( uint16_t localPort );

	void exec();

private:
	uint32_t crc32Stm( uint32_t crc, uint32_t* data, uint32_t size );

private:
	UdpSocket udpSocket;
	uint8_t tmp[2048];
};