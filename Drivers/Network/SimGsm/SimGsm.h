#pragma once

#include "Core/ByteRingBuffer.h"
#include "Core/NanoList.h"

#include "Network/AbstractGsmModem.h"
#include "Drivers/Interfaces/Usart.h"
#include "Drivers/LogicOutput.h"

#include "Network/AbstractUdpSocket.h"
#include "Network/AbstractTcpSocket.h"
#include "SimGsmTcpServer.h"
#include "SimGsmATResponseParsers.h"

#define SIMGSM_SOCKET_LIMIT      6
#define SIMGSM_ENABLE_LEVEL      Level::Low
#define SIMGSM_USE_PWRKEY_PIN

class SimGsmTcpServer;
class SimGsmUdpSocket;
class SimGsmTcpSocket;

class SimGsm : private BaseStaticThread< 1280 >, public AbstractGsmModem
{
	friend class SimGsmSocketBase;
	friend class SimGsmUdpSocket;
	friend class SimGsmTcpSocket;
	friend class SimGsmTcpServer;

public:
	SimGsm( IOPort enablePort );
	~SimGsm();

	void setUsart( Usart* usart );
	void setPinCode( uint32_t pinCode ) final override;
	void setApn( const char* apn ) final override;

	void startModem( tprio_t prio ) final override;
	void stopModem() final override;
	bool waitForStatusChange( sysinterval_t timeout = TIME_INFINITE ) final override;
	ModemStatus status() final override;
	ModemError modemError() final override;
	IpAddress networkAddress() final override;

	AbstractTcpServer* tcpServer( uint32_t index ) final override;

	AbstractUdpSocket* createUdpSocket() final override; // reentrant 
	AbstractUdpSocket* createUdpSocket( uint32_t inputBufferSize, uint32_t outputBufferSize ) final override; // reentrant 

	AbstractTcpSocket* createTcpSocket() final override; // reentrant 
	AbstractTcpSocket* createTcpSocket( uint32_t inputBufferSize, uint32_t outputBufferSize ) final override; // reentrant 

	EvtSource* eventSource() final override;

private:
	void main() final override;

	enum class InitResult { Ok, PinCodeIncorrect, Fail };
	InitResult reinit();
	int executeSetting( uint32_t lexicalAnalyzerState, const char* cmd, uint32_t delayMs = 100, uint32_t repeateCount = 8, sysinterval_t timeout = TIME_S2I( 5 ) );
	void waitForCPinOrPwrDown();
	void waitForReadyFlags(); // Call and Sms

	void lexicalAnalyzerCallback( LexicalAnalyzer::ParsingResult* res );
	inline void generalRequestHandler( LexicalAnalyzer::ParsingResult* res );
	inline void sendRequestHandler( LexicalAnalyzer::ParsingResult* res );
	inline void startRequestHandler( LexicalAnalyzer::ParsingResult* res );
	inline void closeRequestHandler( LexicalAnalyzer::ParsingResult* res );
	inline void serverRequestHandler( LexicalAnalyzer::ParsingResult* res );
	inline void atRequestHandler( LexicalAnalyzer::ParsingResult* res );

	struct Request;
	typedef NanoList< Request* >::Node RequestNode;
	bool addRequest( RequestNode* requestNode );
	bool addRequestS( RequestNode* requestNode );
	void nextRequest();
	void completePacketTransfer( bool errorFlag );
	void moveSendRequestToBackS();

	bool openUdpSocket( AbstractUdpSocket* socket, uint16_t bindPort );
	bool openTcpSocket( AbstractTcpSocket* socket, IpAddress remoteAddress, uint16_t remotePort );
	uint32_t sendSocketData( AbstractUdpSocket* socket, const uint8_t* data, uint16_t size, IpAddress remoteAddress, uint16_t remotePort );
	uint32_t sendSocketData( AbstractTcpSocket* socket, const uint8_t* data, uint16_t size );
	void closeSocket( AbstractSocket* socket );
	bool serverStart( uint16_t port );
	void serverStop();

	inline void sendCommand( uint32_t len, sysinterval_t timeout = TIME_S2I( 1 ) );
	void send( const uint8_t* dataFirst, uint32_t sizeFirst, const uint8_t* dataSecond, uint32_t sizeSecond );
	inline void nextSend();

	void closeAllS();
	void crash();
	void shutdown();

	static void timeoutCallback( void* );
	static void modemPingCallback( void* );

	int reserveLink();
	inline void dereserveLink( int id );

	inline int printCSTT();
	inline int printCPIN();
	inline int printCLPORT( int linkId, uint16_t port );
	inline int printCIPCLOSE( int linkId );
	inline int printCIPUDPMODE2( int linkId, IpAddress addr, uint16_t port );
	inline int printCIPSERVER( uint16_t port );
	inline int printAT();
	int printCIPSTART( int linkId, SocketType type, IpAddress addr, uint16_t port );
	int printCIPSEND( int linkId, uint32_t dataSize );

private:
	enum InnerEventFlag : eventflags_t { StopRequestFlag = 1, TimeoutEventFlag = 2, NewRequestEventFlag = 4, ModemPingEventFlag = 8 };
	enum ErrorCode { NoError, TimeoutError, OverflowError, GeneralError, CMEError };
	enum class SendRequestState { SetRemoteAddress, SendInit, SendData };

	ModemStatus mStatus;
	ModemError mError;
	IpAddress netAddress;
	EvtSource extEventSource;
	EvtSource innerEventSource;
	threads_queue_t waitingQueue;
	virtual_timer_t responseTimer;
	virtual_timer_t modemPingTimer;

	Usart* usart;
	LogicOutput enablePin;
	uint32_t pinCode;
	const char* apn;
	IpAddress ip;
	volatile bool pwrDown, crashFlag, callReady, smsReady;
	SimGsmATResponseParsers::CPinParsingResult::Status cpinStatus;
	int sendReqErrorLinkId;
	SendRequestState sendReqState;
	systime_t tcpSocketReqTime;

	struct LinkDesc
	{
		AbstractSocket* socket;
		volatile bool reserved;
	} linkDesc[SIMGSM_SOCKET_LIMIT];
	SimGsmTcpServer* server;

	LexicalAnalyzer lexicalAnalyzer;
	SimGsmATResponseParsers::CPinParser cpinParser;
	SimGsmATResponseParsers::ReceiveParser receiveParser;
	SimGsmATResponseParsers::RemoteIpParser remoteIpParser;
	SimGsmATResponseParsers::IpParser ipParser;
	SimGsmATResponseParsers::ClosedParser closedParser;
	SimGsmATResponseParsers::CloseOkParser closeOkParser;
	SimGsmATResponseParsers::AlreadyConnectParser alreadyConnectParser;
	SimGsmATResponseParsers::ConnectOkParser connectOkParser;
	SimGsmATResponseParsers::ConnectFailParser connectFailParser;
	SimGsmATResponseParsers::CMEErrorParser cmeErrorParser;

	struct Request
	{
		const enum class Type { General, At, Start, Send, Close, Server } type;
		binary_semaphore_t semaphore;

		Request( Type type ) : type( type )
		{
			chBSemObjectInit( &semaphore, true );
		}
	};
	struct GeneralRequest : public Request
	{
		GeneralRequest() : Request( Type::General ), status( Status::Executing ), errCode( -1 ) {}
		enum class Status { Executing, Ok, Error } status;
		int errCode;
	};
	struct AtRequest : public Request
	{
		AtRequest() : Request( Request::Type::At ) { use = false; }
		volatile bool use;
	};
	struct StartRequest : public Request
	{
		StartRequest( AbstractSocket* socket, uint16_t localPort, IpAddress remoteAddress, uint16_t remotePort ) : Request( Request::Type::Start )
		{
			this->socket = socket;
			this->localPort = localPort;
			this->remotePort = remotePort;
			this->remoteAddress = remoteAddress;
			linkId = -1;
		}
		AbstractSocket* socket;
		uint16_t localPort;
		uint16_t remotePort;
		IpAddress remoteAddress;
		int linkId;
	};
	struct SendRequest : public Request
	{
		SendRequest() : Request( Request::Type::Send )
		{
			socket = nullptr;
			remoteAddress = IpAddress( 0, 0, 0, 0 );
			remotePort = 0;
			data[0] = data[1] = nullptr;
			dataSize[0] = dataSize[1] = 0;
			use = false;
		}

		AbstractSocket* socket;
		IpAddress remoteAddress;
		uint16_t remotePort;
		volatile bool use;
		const uint8_t* data[2];
		uint16_t dataSize[2];
	};
	struct CloseRequest : public Request
	{
		CloseRequest( AbstractSocket* socket ) : Request( Request::Type::Close )
		{
			this->socket = socket;
		}
		AbstractSocket* socket;
	};
	struct ServerRequest : public Request
	{
		ServerRequest( uint16_t port ) : Request( Request::Type::Server )
		{
			this->port = port;
		}
		uint16_t port;
	};
	NanoList< Request* > requestList;
	RequestNode* currentRequest;
	AtRequest atReq;
	RequestNode atReqNode;

	char str[70];
	const uint8_t* dataSend[2];
	uint32_t dataSendSize[2];
};