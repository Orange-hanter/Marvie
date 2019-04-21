#pragma once

#include "ModbusServer.h"
#include "Network/TcpServer.h"

#define TCP_MODBUS_SERVER_STACK_SIZE  1024

class TcpModbusServer : public AbstractModbusNetworkServer
{
public:
	TcpModbusServer( uint32_t stackSize = TCP_MODBUS_SERVER_STACK_SIZE );
	~TcpModbusServer();

private:
	void main() final override;
	void addNewClient( TcpSocket* socket );
	static void timerCallback( void* p );

private:
	struct SocketStream : public ModbusPotato::IStream
	{
		TcpSocket* socket;
		SocketStream( TcpSocket* socket );
		~SocketStream();

		int read( uint8_t* buffer, size_t buffer_size );
		int write( uint8_t* buffer, size_t len );
		void txEnable( bool state );
		bool writeComplete();
		void communicationStatus( bool rx, bool tx );
	};
	struct Client 
	{
		Client();
		~Client();
		SocketStream* stream;
		uint8_t* buffer;
		ModbusPotato::IFramer* framer;
		EventListener listener;
		virtual_timer_t timer;
	}** clients;
	TcpServer tcpServer;
	EventListener listener;
};