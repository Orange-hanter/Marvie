#pragma once

#include "CommandLineParser.h"
#include "Core/Event.h"
#include "Core/RingBuffer.h"
#include "Core/Semaphore.h"
#include "Core/Thread.h"
#include "Core/ThreadsQueue.h"
#include "Core/Concurrent.h"
#include "FileSystem/File.h"
#include "FileSystem/Dir.h"
#include <map>

class RemoteTerminalServer : private Thread
{
public:
	enum class State
	{
		Stopped,
		WaitSync,
		Working,
		Stopping
	};
	enum class Event { StateChanged = 1 };

	class Terminal
	{
		friend class RemoteTerminalServer;
		Terminal( RemoteTerminalServer* server );

	public:
		int stdOutWrite( const uint8_t* data, uint32_t size );
		int stdErrWrite( const uint8_t* data, uint32_t size );
		int readKeyCode();
		int readLine( uint8_t* data, uint32_t maxSize );
		int readBytes( uint8_t* data, uint32_t maxSize );
		bool isTerminationRequested();

		void setCursorReplaceMode( bool enabled );
		void moveCursor( int n );
		void moveCursorToEnd();
		void moveCursorToBegin();
		void moveCursorToEndOfLine();
		void moveCursoreToBeginOfLine();

		void deleteNextSymbol();
		void deletePrevSymbol();
		void clear();
		void setTextColor( uint8_t r, uint8_t g, uint8_t b );
		void setBackgroundColor( uint8_t r, uint8_t g, uint8_t b );
		void restoreColors();

	private:
		RemoteTerminalServer* server;
	};

	RemoteTerminalServer();
	RemoteTerminalServer( const RemoteTerminalServer& other ) = delete;
	RemoteTerminalServer( RemoteTerminalServer&& other ) = delete;
	~RemoteTerminalServer();

	using OutputCallback = void( * )( const uint8_t* data, uint32_t len, void* p );
	using Function = int( * )( Terminal* terminal, int argc, char* argv[] );
	bool registerFunction( std::string name, Function function );
	void setOutputCallback( OutputCallback callback, void* p = nullptr );
	void setPrefix( std::string prefix );

	bool startServer( tprio_t prio = NORMALPRIO );
	void stopServer();
	bool waitForStopped( sysinterval_t timeout = TIME_INFINITE );
	void reset();
	State state();

	void input( uint8_t* data, uint32_t size );

	EventSourceRef eventSource();

	enum VKey : uint16_t
	{
		VKeyEscape = 256,
		VKeyTab,
		VKeyBacktab,
		VKeyBackspace,
		VKeyReturn,
		VKeyEnter,
		VKeyInsert,
		VKeyDelete,
		VKeyPause,
		VKeyPrint,
		VKeySysReq,
		VKeyClear,
		VKeyHome,
		VKeyEnd,
		VKeyLeft,
		VKeyUp,
		VKeyRight,
		VKeyDown,
		VKeyPageUp,
		VKeyPageDown,
		VKeyShift,
		VKeyControl,
		VKeyMeta,
		VKeyAlt,
		VKeyCapsLock,
		VKeyNumLock,
		VKeyScrollLock,
		VKeyF1,
		VKeyF2,
		VKeyF3,
		VKeyF4,
		VKeyF5,
		VKeyF6,
		VKeyF7,
		VKeyF8,
		VKeyF9,
		VKeyF10,
		VKeyF11,
		VKeyF12,
	};

private:
	enum class TerminalState;
	enum Control : uint8_t;
	void main() override;
	inline void inputHandler();
	inline void programFinishedHandler();
	inline void sendControl( Control c );
	inline void sendData( const uint8_t* data, uint32_t size );
	inline void sendString( const std::string& str );
	template< size_t N >
	inline void sendString( const char( &str )[N] )
	{
		outputCallback( ( uint8_t* )str, N - 1, outputCallbackPrm );
	}
	inline void setState( TerminalState state );
	void removeFiles();
	bool makePath( const char* filePath );
	int echo( Terminal* terminal, int argc, char* argv[] );
	int clear( Terminal* terminal, int argc, char* argv[] );

private:
	friend class Terminal;
	enum : eventmask_t { StopRequestEvent = 1, ResetRequestEvent = 2, InputEvent = 4, ProgramFinishedEvent = 8 };
	enum class TerminalState
	{
		WaitSync,
		WaitCommand,
		Exec,
		WaitAck,
		Stopping,
		Stopped
	} terminalState;
	Terminal terminal;
	std::string prefix;
	OutputCallback outputCallback;
	void* outputCallbackPrm;
	std::map< std::string, Function > functionMap;
	volatile bool controlByte;
	volatile bool syncReceived;
	volatile bool terminationReqReceived;
	std::string line;
	CommandLineParser parser;
	int commandIndex;
	Future< int > future;
	EventListener futureListener;
	StaticRingBuffer< uint8_t, 512 > in;
	File *inFile, *outFile, *errFile;
	int internalCommandResult;
	volatile bool internalCommandFlag;
	EventSource evtSource;
	ThreadsQueue waitingQueue;

	uint8_t* inputData;
	uint32_t inputDataSize;
	BinarySemaphore inputSem;

	enum Control : uint8_t
	{
		Sync = 255,
		Prefix = 1,
		CommandNotFound,
		Ok,
		Error,
		ProgramStarted,
		TerminateRequest,
		ProgramFinished,
		ProgramFinishedAck,

		CursorReplaceModeEnable,
		CursorReplaceModeDisable,
		CursorMoveLeft,
		CursorMoveRight,
		CursorMoveN,
		CursorMoveToEnd,
		CursorMoveToBegin,
		CursorMoveToEndOfLine,
		CursorMoveToBeginOfLine,

		ConsoleDelete,
		ConsoleBackspace,
		ConsoleClear,
		ConsoleSetTextColor,
		ConsoleSetBackgroundColor,
		ConsoleRestoreColors,
		ConsoleCommit,

		KeyEscape = 100,
		KeyTab,
		KeyBacktab,
		KeyBackspace,
		KeyReturn,
		KeyEnter,
		KeyInsert,
		KeyDelete,
		KeyPause,
		KeyPrint,
		KeySysReq,
		KeyClear,
		KeyHome,
		KeyEnd,
		KeyLeft,
		KeyUp,
		KeyRight,
		KeyDown,
		KeyPageUp,
		KeyPageDown,
		KeyShift,
		KeyControl,
		KeyMeta,
		KeyAlt,
		KeyCapsLock,
		KeyNumLock,
		KeyScrollLock,
		KeyF1,
		KeyF2,
		KeyF3,
		KeyF4,
		KeyF5,
		KeyF6,
		KeyF7,
		KeyF8,
		KeyF9,
		KeyF10,
		KeyF11,
		KeyF12,
		Char255
	};
};