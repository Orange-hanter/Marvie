#pragma once

#include "Core/Event.h"
#include "Core/Thread.h"
#include "Core/ThreadsQueue.h"
#include "Core/Timer.h"
#include "FileSystem/File.h"
#include "Network/TcpServer.h"
#include <memory>

class FirmwareTransferService : public Thread
{
public:
	enum class ServiceState { Stopped, Working, Stopping };
	enum class Error { NoError, ServerInnerError };
	enum class Event { StateChanged = 1, Error = 2 };
	class Callback
	{
	public:
		virtual const char* firmwareVersion() { return ""; }
		virtual const char* bootloaderVersion() { return ""; }
		virtual void firmwareDownloaded( const std::string& fileName ) {};
		virtual void bootloaderDownloaded( const std::string& fileName ) {};
		virtual void restartDevice() {};
	};

	FirmwareTransferService();
	FirmwareTransferService( const FirmwareTransferService& other ) = delete;
	FirmwareTransferService( FirmwareTransferService&& other ) = delete;
	~FirmwareTransferService();

	template< typename T >
	void setPassword( T&& password )
	{
		if( mState != State::Stopped )
			return;
		this->password = std::forward< T >( password );
	}

	template< typename T >
	void setPath( T&& path )
	{
		if( mState != State::Stopped )
			return;
		this->path = std::forward< T >( path );
		if( this->path.empty() || this->path.back() != '/' )
			this->path.push_back( '/' );
	}

	void setCallback( Callback* callback );
	ServiceState state() const;
	Error error() const;

	bool startService( tprio_t prio = NORMALPRIO );
	void stopService();
	bool waitForStopped( sysinterval_t timeout = TIME_INFINITE );

	EventSourceRef eventSource();

private:
	void main();
	void processMainSocket();
	void removeSocket();
	enum class State;
	void setState( State newState );
	void setError( Error err );
	void timerCallback();

private:
	enum : eventmask_t
	{
		StopRequestEvent = 1,
		ServerEvent = 2,
		SocketEvent = 4,
		TimerEvent = 8
	};
	enum : sysinterval_t
	{
		AuthTimeout = TIME_MS2I( 10000 ),
		CloseTimeout = TIME_MS2I( 10000 ),
		InactiveTimeout = TIME_S2I( 60 )
	};
	enum class State
	{
		Stopped,
		Listening,
		WaitHeader,
		Authorization,
		WaitCommand,
		WaitFirmwareDesc,
		WaitBootloaderDesc,
		ReceiveFirmware,
		ReceiveBootloader,
		WaitClose,
		Stopping
	} mState;
	enum Command
	{
		AuthOk,
		AuthFailed,
		StartFirmwareTransfer,
		StartBootloaderTransfer,
		TransferOk,
		TransferFailed,
		Restart
	};
	Error err;
	std::string password, path;
	Callback* callback;
	BasicTimer< decltype( &FirmwareTransferService::timerCallback ), &FirmwareTransferService::timerCallback > timer;
	TcpServer server;
	TcpSocket* socket;
	EventListener socketListener;
	File file;
	uint8_t buffer[512];
	uint32_t n;
	ThreadsQueue waitingQueue;
	EventSource evtSource;

	static const char nameStr[25];
	static constexpr uint32_t version = 1;
#pragma pack( push, 1 )
	struct Header
	{
		char name[sizeof( nameStr )];
		uint32_t version;
		uint32_t reserved;
	};
#pragma pack( pop )
	struct Desc
	{
		uint8_t hash[20];
		uint32_t size;
	};
};