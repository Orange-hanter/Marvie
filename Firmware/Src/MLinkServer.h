#pragma once

#include "Core/BaseDynamicThread.h"
#include "Core/ByteRingBuffer.h"
#include "Core/NanoList.h"
#include "Network/TcpServer.h"
#include <list>

#define MLINK_STACK_SIZE           1280

class MLinkServer : private BaseDynamicThread
{
public:
	enum class State { Stopped, Listening, Authenticating, Connected, Stopping };
	enum class Error { NoError, ServerInnerError, TimeoutError, AuthenticationError, BufferOverflowError, RemoteClosedError };
	enum class Event { Error = 1, StateChanged = 2, NewPacketAvailable = 4 };
	class DataChannel
	{
		friend class MLinkServer;
		DataChannel( MLinkServer* );

	public:
		~DataChannel();

		bool open( uint8_t channel, const char* name, uint32_t size = 0 );
		bool sendData( const uint8_t* data, uint32_t size );
		bool sendDataAndClose( const uint8_t* data, uint32_t size );
		void close();
		void cancelAndClose();

	private:
		void close( bool cancelFlag );

		MLinkServer* link;
		NanoList< DataChannel* >::Node node;
		uint32_t id;
		uint8_t ch;
	};
	class DataChannelCallback 
	{
	public:
		virtual bool onOpennig( uint8_t channel, const char* name, uint32_t size ) = 0;
		virtual bool newDataReceived( uint8_t channel, const uint8_t* data, uint32_t size ) = 0;
		virtual void onClosing( uint8_t channel, bool canceled ) = 0;
	};
	class AuthenticationCallback
	{
	public:
		virtual int authenticate( char* accountName, char* password ) = 0;
	};

	MLinkServer();
	~MLinkServer();

	State state() const;
	Error error() const;

	void setAuthenticationCallback( AuthenticationCallback* callback );
	void startListening( tprio_t prio );
	void stopListening();
	bool waitForStateChanged( sysinterval_t timeout = TIME_INFINITE );

	DataChannel* createDataChannel();
	void setDataChannelCallback( DataChannelCallback* callback );

	void confirmSession();
	int accountIndex();
	bool sendPacket( uint8_t type, const uint8_t* data, uint16_t size ); // 0 <= size <= MSS
	bool waitPacket( sysinterval_t timeout = TIME_INFINITE );
	bool hasPendingPacket();
	uint32_t readPacket( uint8_t* type, uint8_t* data, uint32_t maxSize );

	EvtSource* eventSource();

private:
	void main() final override;
	inline void processNewBytes();
	inline void processNewPacketM();
	inline uint32_t socketWriteM( const uint8_t* data, uint32_t size );
	inline void sendAuthAckM();
	inline void sendAuthFailM();
	inline void sendIAmAlive();
	void sendRemoteCloseChannelM( uint8_t channel, uint32_t id );
	bool checkInputDataChM( uint8_t channel, uint32_t id );
	void addInputDataChM( uint8_t channel, uint32_t id );
	void removeInputDataChM( uint8_t channel );
	inline bool checkOutputDataChM( uint8_t channel );
	inline void addOutputDataChM( NanoList< DataChannel* >::Node* node );
	inline void removeOutputDataChM( uint8_t channel );
	inline void closeOutputDataChM( uint8_t channel, uint32_t id );
	inline void timeout();
	inline void closeLink( Error );
	void closeLinkM( Error );
	
	static void timerCallback( void* );
	static void pingTimerCallback( void* );

private:
	friend class DataChannel;
	enum PacketType : uint8_t { Auth, AuthAck, AuthFail, IAmAlive, OpenChannel, ChannelData, CloseChannel, RemoteCloseChannel, User };
	enum InnerEventMask : eventflags_t { StopRequestFlag = 1, TimeoutEventFlag = 2, PingEventFlag = 4, ServerEventFlag = 8, SocketEventFlag = 16 };
#pragma pack( push, 1 )
	struct Header
	{
		uint32_t preamble;
		uint16_t size;
		uint8_t type;
	};
	struct ChannelHeader
	{
		uint32_t id;
		uint8_t ch;
	};
	struct BufferHeader 
	{
		uint16_t size;
		uint8_t type;
	};
#pragma pack( pop )
	enum
	{
		MTU = 1400,
		MSS = MTU - sizeof( Header ),
		DataChannelMSS = MTU - sizeof( Header ) - sizeof( ChannelHeader ),
		PacketBufferSize = ( MSS + sizeof( BufferHeader ) ) * 2,
		MaxPacketTransferInterval = 2500,
		PingInterval = 1500, // must be <= MaxPacketTransferInterval
	};
	State linkState;
	Error linkError;
	TcpServer server;
	TcpSocket* socket;
	EvtListener socketListener;
	AuthenticationCallback* authCallback;
	DataChannelCallback* inputDataChCallback;
	uint32_t idCounter;
	int accountId;
	bool sessionConfirmed;
	threads_queue_t stateWaitingQueue;
	threads_queue_t packetWaitingQueue;
	mutex_t ioMutex;
	virtual_timer_t timer, pingTimer;
	EvtSource extEventSource;
	NanoList< DataChannel* > activeODCList; // output data channel list
	struct IDC
	{
		uint8_t ch;
		uint32_t id;
	};
	std::list< IDC > activeIDCList; // input data channel list
	StaticByteRingBuffer< PacketBufferSize > packetBuffer;

	Header header;
	uint8_t packetData[MTU];
	enum class ParserState { WaitHeader, WaitData } parserState;
};