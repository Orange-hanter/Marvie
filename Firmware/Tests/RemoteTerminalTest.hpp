#include "Core/Assert.h"
#include "Core/Concurrent.h"
#include "Core/MemoryStatus.h"
#include "Network/TcpServer.h"
#include "RemoteTerminal/RemoteTerminalServer.h"
#include "RemoteTerminal/CommandLineUtility.h"

namespace RemoteTerminalTest
{
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

	int test()
	{
		auto mem0 = MemoryStatus::freeSpace( MemoryStatus::Region::All );
		{
			CommandLineParser parser;
			assert( parser.parse( "commandA -t gg='42.42''I\\'m'<'/Path A/in.txt'|commandB>>'/Path B/out.txt'2>&1&commandC&&commandD||commandE" ) == true );
			assert( parser.commandCount() == 5 );
			auto com = parser.command( 0 );
			assert( std::string( com->name() ) == "commandA" );
			assert( com->argc() == 4 );
			assert( std::string( com->argv()[0] ) == "-t" );
			assert( std::string( com->argv()[1] ) == "gg=" );
			assert( std::string( com->argv()[2] ) == "42.42" );
			assert( std::string( com->argv()[3] ) == "I'm" );
			assert( com->stdIn() == CommandLineParser::StdIn::File );
			assert( com->stdOut() == CommandLineParser::StdOut::Pipeline );
			assert( com->stdErr() == CommandLineParser::StdErr::Terminal );
			assert( std::string( com->inFileName() ) == "/Path A/in.txt" );
			assert( com->outFileName() == nullptr );
			assert( com->errFileName() == nullptr );
			assert( com->launchCondition() == CommandLineParser::LaunchCondition::Unconditional );

			com = parser.command( 1 );
			assert( std::string( com->name() ) == "commandB" );
			assert( com->argc() == 0 );
			assert( com->stdIn() == CommandLineParser::StdIn::Pipeline );
			assert( com->stdOut() == CommandLineParser::StdOut::FileAppend );
			assert( com->stdErr() == CommandLineParser::StdErr::StdOut );
			assert( com->inFileName() == nullptr );
			assert( std::string( com->outFileName() ) == "/Path B/out.txt" );
			assert( com->errFileName() == nullptr );
			assert( com->launchCondition() == CommandLineParser::LaunchCondition::Unconditional );

			com = parser.command( 2 );
			assert( std::string( com->name() ) == "commandC" );
			assert( com->argc() == 0 );
			assert( com->stdIn() == CommandLineParser::StdIn::Keyboard );
			assert( com->stdOut() == CommandLineParser::StdOut::Terminal );
			assert( com->stdErr() == CommandLineParser::StdErr::Terminal );
			assert( com->inFileName() == nullptr );
			assert( com->outFileName() == nullptr );
			assert( com->errFileName() == nullptr );
			assert( com->launchCondition() == CommandLineParser::LaunchCondition::Unconditional );

			com = parser.command( 3 );
			assert( std::string( com->name() ) == "commandD" );
			assert( com->argc() == 0 );
			assert( com->stdIn() == CommandLineParser::StdIn::Keyboard );
			assert( com->stdOut() == CommandLineParser::StdOut::Terminal );
			assert( com->stdErr() == CommandLineParser::StdErr::Terminal );
			assert( com->inFileName() == nullptr );
			assert( com->outFileName() == nullptr );
			assert( com->errFileName() == nullptr );
			assert( com->launchCondition() == CommandLineParser::LaunchCondition::PrevSucceed );

			com = parser.command( 4 );
			assert( std::string( com->name() ) == "commandE" );
			assert( com->argc() == 0 );
			assert( com->stdIn() == CommandLineParser::StdIn::Keyboard );
			assert( com->stdOut() == CommandLineParser::StdOut::Terminal );
			assert( com->stdErr() == CommandLineParser::StdErr::Terminal );
			assert( com->inFileName() == nullptr );
			assert( com->outFileName() == nullptr );
			assert( com->errFileName() == nullptr );
			assert( com->launchCondition() == CommandLineParser::LaunchCondition::PrevFail );

			assert( parser.parse( "commandA &|" ) == false );
			assert( parser.errorPosition() == 10 );
			assert( strcmp( parser.errorString(), "Invalid command name" ) == 0 );

			assert( parser.parse( "commandA > out.txt | commandB" ) == false );
			assert( parser.errorPosition() == 19 );
			assert( strcmp( parser.errorString(), "Double redirection" ) == 0 );

			assert( parser.parse( "commandA | commandB < in.txt" ) == false );
			assert( parser.errorPosition() == 20 );
			assert( strcmp( parser.errorString(), "Double redirection" ) == 0 );

			assert( parser.parse( "commandA > out.txt < input.txt > out2.txt" ) == false );
			assert( parser.errorPosition() == 31 );
			assert( strcmp( parser.errorString(), "Double redirection" ) == 0 );

			assert( parser.parse( "commandA -t1 -t2 -t3 -t4 -t5 2> &1 >> out.txt " ) == false );
			assert( parser.errorPosition() == 32 );
			assert( strcmp( parser.errorString(), "Syntax error" ) == 0 );

			assert( parser.parse( "" ) == true );
			assert( parser.commandCount() == 0 );
		}
		assert( mem0 == MemoryStatus::freeSpace( MemoryStatus::Region::All ) );

		palSetPadMode( GPIOC, 8, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
		palSetPadMode( GPIOC, 9, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
		palSetPadMode( GPIOC, 10, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
		palSetPadMode( GPIOC, 11, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
		palSetPadMode( GPIOC, 12, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
		palSetPadMode( GPIOD, 2, PAL_MODE_ALTERNATE( 0x0C ) | PAL_STM32_OSPEED_HIGHEST );
		static SDCConfig sdcConfig;
		sdcConfig.scratchpad = nullptr;
		sdcConfig.bus_width = SDC_MODE_4BIT;
		sdcStart( &SDCD1, &sdcConfig/*nullptr*/ );
		assert( sdcConnect( &SDCD1 ) == HAL_SUCCESS );

		FileSystem* fs = new FileSystem;
		assert( fs->mount( "/" ) );
		Dir( "/test" ).removeRecursively();
		assert( Dir( "/test" ).mkdir() == true );

		auto mem1 = MemoryStatus::freeSpace( MemoryStatus::Region::All );
		RemoteTerminalServer* terminalServer = new RemoteTerminalServer;
		terminalServer->startServer();
		terminalServer->stopServer();
		terminalServer->waitForStopped();
		terminalServer->startServer();
		delete terminalServer;
		assert( mem1 == MemoryStatus::freeSpace( MemoryStatus::Region::All ) );

		//RemoteTerminalServer::OutputCallback callback;
		//uint8_t* buffer;
		static uint8_t* buffer = new uint8_t[512];
		static uint32_t offset = 0;
		terminalServer = new RemoteTerminalServer;
		RemoteTerminalServer::OutputCallback callback = []( const uint8_t* data, uint32_t len, void* p )
		{
			assert( p == buffer );
			assert( len + offset <= 512 );
			memcpy( buffer + offset, data, len );
			offset += len;
		};
		terminalServer->setOutputCallback( callback, buffer );
		terminalServer->registerFunction( "test", []( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] ) -> int
		{
			assert( argc == 1 );
			assert( strlen( argv[0] ) == 2 );
			assert( argv[0][0] = '-' );
			uint8_t data[5];
			assert( terminal->readBytes( data, 5 ) == 5 );
			assert( strncmp( ( char* )data, "12345", 5 ) == 0 );
			terminal->stdOutWrite( ( uint8_t* )"67890", 5 );
			terminal->stdErrWrite( ( uint8_t* )"abcde", 5 );
			if( argv[0][1] == 't' )
				return 0;
			return -1;
		} );
		terminalServer->startServer();
		assert( terminalServer->state() == RemoteTerminalServer::State::WaitSync );
		{
			uint8_t in[] = { 255, Control::Sync };
			terminalServer->input( in, 2 );
			uint8_t out[] = { 255, Control::Sync, '>', ' ', '\n' };
			assert( offset == sizeof( out ) );
			assert( strncmp( ( char* )buffer, ( char* )out, offset ) == 0 );
			assert( terminalServer->state() == RemoteTerminalServer::State::Working );
			offset = 0;
			terminalServer->input( in, 2 );
			assert( offset == sizeof( out ) );
			assert( strncmp( ( char* )buffer, ( char* )out, offset ) == 0 );
			offset = 0;
		}
		{
			char in[] = "echo 12345 | test -t > test/out.txt 2> test/err.txt & echo 12345 > test/in.txt & test -t < test/in.txt > test/out_err.txt 2> &1 && test -f < test/in.txt || echo gg || echo 42\n";
			terminalServer->input( ( uint8_t* )in, sizeof( in ) - 1 );
			Thread::sleep( TIME_MS2I( 1000 ) );
			uint8_t out[] = { 255, Control::ProgramStarted, 255, Control::ConsoleCommit, 255, Control::ConsoleCommit, 255, Control::ConsoleCommit, 255, Control::ConsoleCommit, '6', '7', '8', '9', '0', 'a', 'b', 'c', 'd', 'e', 255, Control::ConsoleCommit, 'g', 'g', 255, Control::ProgramFinished };
			assert( offset == sizeof( out ) );
			assert( strncmp( ( char* )buffer, ( char* )out, offset ) == 0 );
			offset = 0;

			uint8_t data[10];
			File* file = new File;
			file->setFileName( "test/out.txt" );
			assert( file->open( FileSystem::OpenModeFlag::OpenExisting | FileSystem::OpenModeFlag::Read ) == true );
			assert( file->size() == 5 );
			file->read( data, 5 );
			assert( strncmp( ( char* )data, "67890", 5 ) == 0 );
			file->close();

			file->setFileName( "test/err.txt" );
			assert( file->open( FileSystem::OpenModeFlag::OpenExisting | FileSystem::OpenModeFlag::Read ) == true );
			assert( file->size() == 5 );
			file->read( data, 5 );
			assert( strncmp( ( char* )data, "abcde", 5 ) == 0 );
			file->close();

			file->setFileName( "test/in.txt" );
			assert( file->open( FileSystem::OpenModeFlag::OpenExisting | FileSystem::OpenModeFlag::Read ) == true );
			assert( file->size() == 5 );
			file->read( data, 5 );
			assert( strncmp( ( char* )data, "12345", 5 ) == 0 );
			file->close();

			file->setFileName( "test/out_err.txt" );
			assert( file->open( FileSystem::OpenModeFlag::OpenExisting | FileSystem::OpenModeFlag::Read ) == true );
			assert( file->size() == 10 );
			file->read( data, 10 );
			assert( strncmp( ( char* )data, "67890abcde", 10 ) == 0 );
			file->close();
			delete file;

			char trash[] = "echo trash | test\n";
			terminalServer->input( ( uint8_t* )trash, sizeof( trash ) - 1 );
			Thread::sleep( TIME_MS2I( 50 ) );
			assert( offset == 0 );

			uint8_t in2[] = { 255, Control::ProgramFinishedAck };
			terminalServer->input( in2, 2 );
			char in3[] = "echo test\n";
			terminalServer->input( ( uint8_t* )in3, sizeof( in3 ) - 1 );
			Thread::sleep( TIME_MS2I( 10 ) );
			uint8_t out2[] = { 255, Control::ProgramStarted, 't', 'e', 's', 't', 255, Control::ProgramFinished };
			assert( offset == sizeof( out2 ) );
			assert( strncmp( ( char* )buffer, ( char* )out2, offset ) == 0 );
			offset = 0;
			terminalServer->input( in2, 2 );
		}
		delete terminalServer;
		delete[] buffer;
		assert( mem1 == MemoryStatus::freeSpace( MemoryStatus::Region::All ) );
		
		terminalServer = new RemoteTerminalServer;
		terminalServer->registerFunction( "test", []( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] ) -> int
		{
			while( true )
			{
				int key = terminal->readKeyCode();
				if( key == -1 )
					break;
				if( key <= 255 )
				{
					uint8_t byte = ( uint8_t )key;
					terminal->stdOutWrite( &byte, 1 );
				}
				else if( key == RemoteTerminalServer::VKey::VKeyReturn || key == RemoteTerminalServer::VKey::VKeyEnter )
				{
					uint8_t byte = ( uint8_t )'\n';
					terminal->stdOutWrite( &byte, 1 );
				}
			}
			
			return 0;
		} );
		terminalServer->registerFunction( "readline", []( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] ) -> int
		{
			uint32_t maxLineSize = 16;
			const char errMsg[] = "invalid argument";
			if( argc == 2 )
			{
				if( strcmp( argv[0], "-m" ) == 0 )
				{
					auto n = atoi( argv[1] );
					if( n <= 0 )
						goto Error;
					maxLineSize = ( uint32_t )n;
				}
				else
					goto Error;
			}
			else if( argc )
			{
			Error:
				terminal->stdErrWrite( ( uint8_t* )errMsg, sizeof( errMsg ) - 1 );
				return -1;
			}

			std::unique_ptr< uint8_t > buffer( new uint8_t[maxLineSize] );
			if( !buffer )
				return -1;

			int line = 0;
			while( true )
			{
				int size = terminal->readLine( buffer.get(), maxLineSize );
				if( size == -1 )
					break;
				char str[20];
				sprintf( str, "\nLine %d: ", ++line );
				
				terminal->stdOutWrite( ( uint8_t* )str, strlen( str ) );
				terminal->stdOutWrite( buffer.get(), size );
				terminal->stdOutWrite( ( uint8_t* )str, 1 );
			}
			
			return 0;
		} );
		terminalServer->registerFunction( "ls", CommandLineUtility::ls );
		terminalServer->registerFunction( "mkdir", CommandLineUtility::mkdir );
		terminalServer->registerFunction( "rm", CommandLineUtility::rm );
		terminalServer->registerFunction( "cat", CommandLineUtility::cat );
		terminalServer->registerFunction( "ping", CommandLineUtility::ping );
		callback = []( const uint8_t* data, uint32_t len, void* p )
		{
			TcpSocket* socket = reinterpret_cast< TcpSocket* >( p );
			socket->write( data, len );
		};

		PingService::instance()->startService();
		TcpServer server;
		server.listen( 55000 );
		buffer = new uint8_t[512];
		while( true )
		{
			server.waitForNewConnection();
			TcpSocket* socket = server.nextPendingConnection();
			terminalServer->setOutputCallback( callback, socket );
			terminalServer->startServer();
			while( socket->waitForReadAvailable( 1, TIME_INFINITE ) )
			{
				uint32_t size = socket->readAvailable();
				if( size > 512 )
					size = 512;
				socket->read( buffer, size );
				terminalServer->input( buffer, size );
			}
			terminalServer->stopServer();
			terminalServer->waitForStopped();
			delete socket;
		}
		delete[] buffer;
		delete terminalServer;

		return 0;
	}
} // namespace RemoteTerminalTest
