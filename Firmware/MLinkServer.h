#pragma once

#include "Core/BaseDynamicThread.h"
#include "Core/ByteRingBuffer.h"
#include "Core/NanoList.h"
#include "Core/IODevice.h"

#define MLINK_MAX_FRAME_SIZE       267
#define MLINK_PACKET_BUFFER_SIZE   MLINK_MAX_FRAME_SIZE * 2
#define MLINK_STACK_SIZE           1280

class MLinkServer : private BaseDynamicThread
{
public:
	enum class State { Stopped, Listening, Authorizing, Connected, Stopping };
	enum class Error { NoError, SequenceViolationError, ResponseTimeoutError, AuthenticationError, BufferOverflowError, IODeviceClosedError };
	enum class Event { Error = 1, StateChanged = 2, NewPacketAvailable = 4 };
	class ComplexDataChannel
	{
		friend class MLinkServer;
		ComplexDataChannel( MLinkServer* );

	public:
		~ComplexDataChannel();

		bool open( uint8_t id, const char* name, uint32_t size = 0 );
		bool sendData( const uint8_t* data, uint32_t size ); // 0 < size
		bool sendDataAndClose( const uint8_t* data, uint32_t size ); // 0 < size
		void close();
		void cancelAndClose();

	private:
		void close( bool cancelFlag );

		struct BeginResponse
		{
			uint8_t id;
			binary_semaphore_t sem;
			uint32_t g;
		};
		struct DataNextResponse
		{
			uint8_t id;
			binary_semaphore_t sem;
			volatile bool nextOk;
		};
		MLinkServer* link;
		uint64_t r;
		uint32_t g, n;
		uint8_t id;
	};
	class ComplexDataCallback 
	{
	public:
		virtual uint32_t onOpennig( uint8_t id, const char* name, uint32_t size ) = 0;
		virtual bool newDataReceived( uint8_t id, const uint8_t* data, uint32_t size ) = 0;
		virtual void onClosing( uint8_t id, bool canceled ) = 0;
	};
	class AuthenticationCallback
	{
	public:
		virtual bool authenticate( char* accountName, char* password ) = 0;
	};

	MLinkServer();
	~MLinkServer();

	State state() const;
	Error error() const;

	void setIODevice( IODevice* device );
	void setIntegrityControlEnabled( bool enabled );
	void setAuthenticationCallback( AuthenticationCallback* callback );
	void startListening( tprio_t prio );
	void stopListening();
	bool waitForStateChanged( sysinterval_t timeout = TIME_INFINITE );

	ComplexDataChannel* createComplexDataChannel();
	void setComplexDataReceiveCallback( ComplexDataCallback* callback );

	void confirmSession();
	bool sendPacket( uint8_t type, const uint8_t* data, uint8_t size ); // 0 <= size <= 255
	bool waitPacket( sysinterval_t timeout = TIME_INFINITE );
	bool hasPendingPacket();
	uint32_t readPacket( uint8_t* type, uint8_t* data, uint32_t maxSize );

	EvtSource* eventSource();

private:
	void main() final override;
	inline void processNewBytes();
	inline void processNewPacket();
	inline void sendFin( uint64_t ackR );
	void sendFinM( uint64_t ackR );
	void sendFinAckM( uint64_t ackR );
	inline void sendPongM();
	inline void sendSynAckM();
	inline void sendAuthM();
	inline void sendComplexDataBeginAckM( uint8_t id, uint32_t g );
	inline void sendComplexDataNextAckM( uint8_t id, bool nextOk );
	inline void timeout();
	inline void closeLink( Error );
	void closeLinkM( Error );
	static uint32_t calcCrc( const uint8_t* data, uint32_t size );
	static uint32_t calcCrc( const uint8_t* pDataA, uint32_t sizeA, const uint8_t* pDataB, uint32_t sizeB );
	static void timerCallback( void* );

private:
	friend class ComplexDataChannel;
	enum PacketType : uint8_t { Syn, SynAck, Auth, AuthAck, Fin, FinAck, Ping, Pong, ComplexDataBegin, ComplexDataBeginAck, ComplexData, ComplexDataNext, ComplexDataNextAck, ComplexDataEnd, User };
	enum InnerEventMask : eventflags_t { StopRequestFlag = 1, TimeoutEventFlag = 2, IODeviceEventFlag = 4 };
	enum
	{
		MaxPacketTransferInterval = 1500,
	};
	State linkState;
	Error linkError;
	IODevice* device;
	volatile bool integrityControlEnabled;
	AuthenticationCallback* authCallback;
	ComplexDataCallback* cdcCallback;
	uint16_t sqNumCounter;
	uint16_t sqNumNext;
	uint64_t r, lastR;
	uint64_t confirmedR;
	threads_queue_t stateWaitingQueue;
	threads_queue_t packetWaitingQueue;
	mutex_t ioMutex, cdcMutex;
	virtual_timer_t timer;
	EvtSource extEventSource;
	ComplexDataChannel::BeginResponse* cdcBResp;
	NanoList< ComplexDataChannel::DataNextResponse* > cdcDataRespList;
	StaticByteRingBuffer< MLINK_PACKET_BUFFER_SIZE > packetBuffer;

	struct Header
	{
		uint32_t preamble;
		uint8_t size;
		uint8_t type;
		uint16_t sqNum;
	};
	uint8_t packetData[MLINK_MAX_FRAME_SIZE];
	enum class ParserState { WaitHeader, WaitData } parserState;
};