#include "RemoteTerminalServer.h"
#include <string.h>

#define REMOTE_TERMINAL_SERVER_DIR_PATH "/Temp/Terminal/"

RemoteTerminalServer::RemoteTerminalServer() : Thread( 1536 ), terminal( this ), inputSem( true )
{
	terminalState = TerminalState::Stopped;
	prefix = "> \n";

	outputCallback = nullptr;
	outputCallbackPrm = nullptr;

	controlByte = false;
	syncReceived = false;
	terminationReqReceived = false;
	commandIndex = -1;
	inFile = outFile = errFile = nullptr;
	internalCommandResult = 0;
	internalCommandFlag = false;

	inputData = nullptr;
	inputDataSize = 0;
}

RemoteTerminalServer::~RemoteTerminalServer()
{
	stopServer();
	waitForStopped();
}

bool RemoteTerminalServer::registerFunction( std::string name, Function function )
{
	if( terminalState != TerminalState::Stopped )
		return false;

	functionMap.emplace( std::move( name ), std::move( function ) );
	return true;
}

void RemoteTerminalServer::setOutputCallback( OutputCallback callback, void* p /*= nullptr */ )
{
	if( terminalState != TerminalState::Stopped )
		return;

	outputCallback = callback;
	outputCallbackPrm = p;
}

void RemoteTerminalServer::setPrefix( std::string prefix )
{
	if( terminalState != TerminalState::Stopped )
		return;

	prefix.erase( prefix.find( '\n' ) );
	this->prefix = prefix + '\n';
}

bool RemoteTerminalServer::startServer( tprio_t prio /*= NORMALPRIO */ )
{
	if( terminalState != TerminalState::Stopped )
		return false;

	terminalState = TerminalState::WaitSync;
	setPriority( prio );
	if( !start() )
	{
		terminalState = TerminalState::Stopped;
		return false;
	}
	evtSource.broadcastFlags( ( eventflags_t )Event::StateChanged );
	return true;
}

void RemoteTerminalServer::stopServer()
{
	CriticalSectionLocker locker;
	if( terminalState == TerminalState::Stopping || terminalState == TerminalState::Stopped )
		return;

	terminalState = TerminalState::Stopping;
	evtSource.broadcastFlags( ( eventflags_t )Event::StateChanged );
	signalEventsI( StopRequestEvent );
}

bool RemoteTerminalServer::waitForStopped( sysinterval_t timeout /*= TIME_INFINITE */ )
{
	CriticalSectionLocker locker;
	msg_t msg = MSG_OK;
	if( terminalState == TerminalState::Stopping )
		msg = waitingQueue.enqueueSelf( timeout );
	return msg == MSG_OK;
}

void RemoteTerminalServer::reset()
{
	CriticalSectionLocker locker;
	if( terminalState == TerminalState::Stopped || terminalState == TerminalState::Stopping )
		return;
	signalEventsI( ResetRequestEvent );
}

RemoteTerminalServer::State RemoteTerminalServer::state()
{
	switch( terminalState )
	{
	case RemoteTerminalServer::TerminalState::WaitSync:
		return State::WaitSync;
	case RemoteTerminalServer::TerminalState::WaitCommand:
		return State::Working;
	case RemoteTerminalServer::TerminalState::Exec:
		return State::Working;
	case RemoteTerminalServer::TerminalState::WaitAck:
		return State::Working;
	case RemoteTerminalServer::TerminalState::Stopping:
		return State::Stopping;
	case RemoteTerminalServer::TerminalState::Stopped:
	default:
		return State::Stopped;
	}
}

void RemoteTerminalServer::input( uint8_t* data, uint32_t size )
{
	CriticalSectionLocker locker;
	if( terminalState == TerminalState::Stopping || terminalState == TerminalState::Stopped )
		return;
	inputData = data;
	inputDataSize = size;
	signalEventsI( InputEvent );
	inputSem.acquire();
}

EventSourceRef RemoteTerminalServer::eventSource()
{
	return evtSource.reference();
}

void RemoteTerminalServer::main()
{
	controlByte = false;
	syncReceived = false;
	terminationReqReceived = false;
	
	while( terminalState != TerminalState::Stopping )
	{
		eventmask_t em = chEvtWaitAny( ALL_EVENTS );
		if( em & StopRequestEvent )
			break;
		if( em & ResetRequestEvent )
		{
			if( terminalState == TerminalState::WaitCommand || terminalState == TerminalState::WaitAck )
			{
				CriticalSectionLocker locker;
				if( terminalState != TerminalState::Stopping )
				{
					terminalState = TerminalState::WaitSync;
					evtSource.broadcastFlags( ( eventflags_t )Event::StateChanged );
				}
			}
			else if( terminalState == TerminalState::Exec )
			{
				terminationReqReceived = true;
				in.clear();
				future.wait();
				future = Future< int >();
				removeFiles();
				em &= ~ProgramFinishedEvent;
				getAndClearEvents( ProgramFinishedEvent );
				CriticalSectionLocker locker;
				if( terminalState != TerminalState::Stopping )
				{
					terminalState = TerminalState::WaitSync;
					evtSource.broadcastFlags( ( eventflags_t )Event::StateChanged );
				}
			}
		}
		if( em & InputEvent )
		{
			inputHandler();
			inputSem.release();
		}
		if( em & ProgramFinishedEvent )
			programFinishedHandler();
	}

	terminationReqReceived = true;
	in.clear();
	future.wait();
	future = Future< int >();
	removeFiles();
	chSysLock();
	terminalState = TerminalState::Stopped;
	evtSource.broadcastFlags( ( eventflags_t )Event::StateChanged );
	waitingQueue.dequeueAll( MSG_OK );
	exitS( MSG_OK );
}

void RemoteTerminalServer::inputHandler()
{
	do
	{
		if( controlByte )
		{
			if( *inputData == Control::Sync )
			{
				controlByte = false;
				syncReceived = true;
				++inputData, --inputDataSize;

				if( terminalState == TerminalState::WaitSync || terminalState == TerminalState::WaitCommand || terminalState ==  TerminalState::WaitAck )
				{
					syncReceived = false;
					{
						CriticalSectionLocker locker;
						if( terminalState != TerminalState::Stopping )
						{
							if( terminalState == TerminalState::WaitSync )
								evtSource.broadcastFlags( ( eventflags_t )Event::StateChanged );
							terminalState = TerminalState::WaitCommand;
						}
					}
					line.clear();
					sendControl( Control::Sync );
					sendString( prefix );
				}
				else if( terminalState == TerminalState::Exec )
				{
					terminationReqReceived = true;
					in.clear();
				}
				continue;
			}
		}
		else
		{
			if( *inputData == 255 )
			{
				controlByte = true;
				++inputData, --inputDataSize;
				continue;
			}
		}

		if( syncReceived )
		{
			++inputData, --inputDataSize;
			continue;
		}

		switch( terminalState )
		{
		case TerminalState::Exec:
		{
			if( controlByte )
			{
				if( *inputData == Control::TerminateRequest )
				{
					terminationReqReceived = true;
					in.clear();
				}
				else if( *inputData >= Control::KeyEscape && *inputData <= Control::Char255 )
				{
					uint8_t esc[2] = { 255, *inputData };
					in.write( esc, 2 );
				}
			}
			else
				in.write( inputData, 1 );
			break;
		}
		case TerminalState::WaitCommand:
		{
			if( *inputData == '\n' )
			{
				if( !parser.parse( line.c_str() ) )
				{
					sendControl( Control::Error );
					std::string err( "<Terminal>: " );
					err += parser.errorString();
					char buf[15];
					sprintf( buf, " at %d\n", parser.errorPosition() );
					err += buf;
					sendString( err );
				}
				else if( parser.commandCount() == 0 )
					sendControl( Control::Ok );
				else
				{
					std::string errName;
					for( int i = 0; i < parser.commandCount(); ++i )
					{
						auto command = parser.command( i );
						if( strcmp( command->name(), "echo" ) != 0 && 
							strcmp( command->name(), "clear" ) != 0 &&
							functionMap.find( command->name() ) == functionMap.end() )
						{
							errName = command->name();
							break;
						}
					}

					if( !errName.empty() )
					{
						sendControl( Control::CommandNotFound );
						errName += "\n";
						sendString( errName );
						parser.clear();
					}
					else
					{
						setState( TerminalState::Exec );
						commandIndex = -1;
						terminationReqReceived = false;
						internalCommandFlag = false;
						addEvents( ProgramFinishedEvent );
						sendControl( Control::ProgramStarted );
					}
				}

				line.clear();
				break;
			}
			else
				line.push_back( *inputData );
			break;
		}
		case TerminalState::WaitAck:
		{
			if( controlByte )
			{
				if( *inputData == Control::ProgramFinishedAck )
					setState( TerminalState::WaitCommand );
			}
			break;
		}	
		default:
			break;
		}
		controlByte = false;
		++inputData;
		--inputDataSize;
	} while( inputDataSize );
}

void RemoteTerminalServer::programFinishedHandler()
{
	if( commandIndex != -1 )
	{
		removeFiles();

		Dir dir( REMOTE_TERMINAL_SERVER_DIR_PATH );
		auto currentCommand = parser.command( commandIndex );
		if( currentCommand->stdIn() == CommandLineParser::StdIn::Pipeline && !dir.remove( "__in" ) )
		{
			sendString( "\n<Terminal>: internal error" );
			goto End;
		}

		int result;
		if( internalCommandFlag )
		{
			result = internalCommandResult;
			internalCommandFlag = false;
		}
		else
			result = future.get();
		if( commandIndex + 1 < parser.commandCount() )
		{
			const auto nextCommand = parser.command( commandIndex + 1 );
			if( ( nextCommand->launchCondition() == CommandLineParser::LaunchCondition::PrevFail && result == 0 ) ||
				( nextCommand->launchCondition() == CommandLineParser::LaunchCondition::PrevSucceed && result != 0 ) )
				goto End;
		}

		if( currentCommand->stdOut() == CommandLineParser::StdOut::Pipeline && !dir.rename( "__out", "__in" ) )
		{
			sendString( "\n<Terminal>: internal error" );
			goto End;
		}
	}
	if( ++commandIndex == parser.commandCount() )
	{
	End:
		future = Future< int >();
		parser.clear();
		setState( TerminalState::WaitAck );
		sendControl( Control::ProgramFinished );
		return;
	}

	if( commandIndex != 0 )
	{
		auto prevCommand = parser.command( commandIndex - 1 );
		if( prevCommand->stdOut() == CommandLineParser::StdOut::Pipeline )
			;

		sendControl( Control::ConsoleCommit );
	}
	else
		Dir( REMOTE_TERMINAL_SERVER_DIR_PATH ).remove( "__in" );
	Dir( REMOTE_TERMINAL_SERVER_DIR_PATH ).mkpath();

	auto command = parser.command( commandIndex );
	if( command->stdIn() == CommandLineParser::StdIn::Pipeline )
	{
		inFile = new File( std::string( REMOTE_TERMINAL_SERVER_DIR_PATH ) + "__in" );
		if( !inFile || !inFile->open( FileSystem::OpenModeFlag::OpenExisting | FileSystem::OpenModeFlag::Read ) )
		{
			removeFiles();
			sendString( "\n<Terminal>: internal error" );
			goto End;
		}
	}
	else if( command->stdIn() == CommandLineParser::StdIn::File )
	{
		inFile = new File( command->inFileName() );
		if( !( inFile )|| !inFile->open( FileSystem::OpenModeFlag::OpenExisting | FileSystem::OpenModeFlag::Read ) )
		{
			removeFiles();
			sendString( std::string( "\n<Terminal>: file \'" ).append( command->inFileName() ).append( "\' does not exist" ) );
			goto End;
		}
	}
	if( command->stdOut() == CommandLineParser::StdOut::Pipeline )
	{
		outFile = new File( std::string( REMOTE_TERMINAL_SERVER_DIR_PATH ) + "__out" );
		if( !outFile || !outFile->open( FileSystem::OpenModeFlag::CreateNew | FileSystem::OpenModeFlag::Write ) )
		{
			removeFiles();
			sendString( "\n<Terminal>: internal error" );
			goto End;
		}
	}
	else if( command->stdOut() == CommandLineParser::StdOut::File )
	{
		if( !makePath( command->outFileName() ) )
		{
			removeFiles();
			sendString( std::string( "\n<Terminal>: invalid file name \'" ).append( command->outFileName() ).append( "\'" ) );
			goto End;
		}
		outFile = new File( command->outFileName() );
		if( !outFile || !outFile->open( FileSystem::OpenModeFlag::CreateAlways | FileSystem::OpenModeFlag::Write ) )
		{
			removeFiles();
			sendString( "\n<Terminal>: internal error" );
			goto End;
		}
	}
	else if( command->stdOut() == CommandLineParser::StdOut::FileAppend )
	{
		if( !makePath( command->outFileName() ) )
		{
			removeFiles();
			sendString( std::string( "\n<Terminal>: invalid file name \'" ).append( command->outFileName() ).append( "\'" ) );
			goto End;
		}
		outFile = new File( command->outFileName() );
		if( !outFile || !outFile->open( FileSystem::OpenModeFlag::OpenAppend | FileSystem::OpenModeFlag::Write ) )
		{
			removeFiles();
			sendString( "\n<Terminal>: internal error" );
			goto End;
		}
	}
	if( command->stdErr() == CommandLineParser::StdErr::File )
	{
		if( !makePath( command->errFileName() ) )
		{
			removeFiles();
			sendString( std::string( "\n<Terminal>: invalid file name \'" ).append( command->errFileName() ).append( "\'" ) );
			goto End;
		}
		errFile = new File( command->errFileName() );
		if( !errFile || !errFile->open( FileSystem::OpenModeFlag::CreateAlways | FileSystem::OpenModeFlag::Write ) )
		{
			removeFiles();
			sendString( "\n<Terminal>: internal error" );
			goto End;
		}
	}
	else if( command->stdErr() == CommandLineParser::StdErr::FileAppend )
	{
		if( !makePath( command->errFileName() ) )
		{
			removeFiles();
			sendString( std::string( "\n<Terminal>: invalid file name \'" ).append( command->errFileName() ).append( "\'" ) );
			goto End;
		}
		errFile = new File( command->errFileName() );
		if( !errFile || !errFile->open( FileSystem::OpenModeFlag::OpenAppend | FileSystem::OpenModeFlag::Write ) )
		{
			removeFiles();
			sendString( "\n<Terminal>: internal error" );
			goto End;
		}
	}
	else if( command->stdErr() == CommandLineParser::StdErr::StdOut )
		errFile = outFile;

	if( strcmp( command->name(), "echo" ) == 0 )
	{
		future = Future< int >();
		internalCommandResult = echo( &terminal, command->argc(), command->argv() );
		internalCommandFlag = true;
		Thread::signalEvents( ProgramFinishedEvent );
		return;
	}
	if( strcmp( command->name(), "clear" ) == 0 )
	{
		future = Future< int >();
		internalCommandResult = clear( &terminal, command->argc(), command->argv() );
		internalCommandFlag = true;
		Thread::signalEvents( ProgramFinishedEvent );
		return;
	}
		
	auto functionIter = functionMap.find( command->name() );
	future = Concurrent::run( ThreadProperties( 2048, NORMALPRIO - 1, command->name() ), ( *functionIter ).second, &terminal, command->argc(), command->argv() );
	if( future.isValid() )
	{
		future.eventSource().registerMask( &futureListener, ProgramFinishedEvent );
		if( future.wait( TIME_IMMEDIATE ) )
			Thread::signalEvents( ProgramFinishedEvent );
	}
	else
	{
		sendString( "\n<Terminal>: internal error" );
		goto End;
	}
}

void RemoteTerminalServer::sendControl( Control c )
{
	const uint8_t data[2] = { 255, ( uint8_t )c };
	outputCallback( data, 2, outputCallbackPrm );
}

void RemoteTerminalServer::sendData( const uint8_t* data, uint32_t size )
{
	outputCallback( data, size, outputCallbackPrm );
}

void RemoteTerminalServer::sendString( const std::string& str )
{
	outputCallback( ( uint8_t* )str.c_str(), str.size(), outputCallbackPrm );
}

void RemoteTerminalServer::setState( TerminalState newState )
{
	CriticalSectionLocker locker;
	if( terminalState != TerminalState::Stopping )
		terminalState = newState;
}

void RemoteTerminalServer::removeFiles()
{
	delete inFile;
	if( outFile == errFile )
		delete outFile;
	else
	{
		delete outFile;
		delete errFile;
	}
	inFile = nullptr;
	outFile = errFile = nullptr;
}


bool RemoteTerminalServer::makePath( const char* filePath )
{
	const char* i;
	for( i = filePath + strlen( filePath ); i != filePath && *i != '/'; --i )
		;
	std::string path( filePath, i - filePath );
	return Dir( path ).mkpath();
}

int RemoteTerminalServer::echo( Terminal* terminal, int argc, char* argv[] )
{
	for( int i = 0; i < argc; ++i )
	{
		if( terminal->stdOutWrite( ( uint8_t* )argv[i], strlen( argv[i] ) ) == -1 )
			return -1;
	}

	return 0;
}

int RemoteTerminalServer::clear( Terminal* terminal, int argc, char* argv[] )
{
	terminal->clear();
	return 0;
}

RemoteTerminalServer::Terminal::Terminal( RemoteTerminalServer* server ) : server( server )
{

}

int RemoteTerminalServer::Terminal::stdOutWrite( const uint8_t* data, uint32_t size )
{
	if( server->terminationReqReceived )
		return -1;
	if( server->outFile )
		return server->outFile->write( data, size );
	else
	{
		auto _size = size;
		while( size )
		{
			const uint8_t* begin = data;
			while( size && *data != 255 )
				++data, --size;
			uint32_t partSize = data - begin;
			if( partSize )
				server->sendData( begin, partSize );
			if( size )
			{
				server->sendControl( RemoteTerminalServer::Control::Char255 );
				++data;
				--size;
			}
		}
		return _size;
	}
}

int RemoteTerminalServer::Terminal::stdErrWrite( const uint8_t* data, uint32_t size )
{
	if( server->terminationReqReceived )
		return -1;
	if( server->errFile )
		return server->errFile->write( data, size );
	else
	{
		auto _size = size;
		while( size )
		{
			const uint8_t* begin = data;
			while( size && *data != 255 )
				++data, --size;
			uint32_t partSize = data - begin;
			if( partSize )
				server->sendData( begin, partSize );
			if( size )
			{
				server->sendControl( RemoteTerminalServer::Control::Char255 );
				++data;
				--size;
			}
		}
		return _size;
	}
}

int RemoteTerminalServer::Terminal::readKeyCode()
{
	if( server->inFile )
	{
		if( server->terminationReqReceived )
			return -1;
		uint8_t byte;
		if( server->inFile->read( &byte, 1 ) )
			return byte;
		return -1;
	}
	else
	{
		CriticalSectionLocker locker;
		if( server->terminationReqReceived )
			return -1;
		uint8_t byte;
		if( server->in.waitForReadAvailable( 1, TIME_INFINITE ) && server->in.read( &byte, 1 ) )
		{
			if( byte == 255 )
			{
				if( server->in.read( &byte, 1 ) )
				{
					if( byte == RemoteTerminalServer::Control::Char255 )
						return 255;
					return ( int )RemoteTerminalServer::VKeyEscape + byte - RemoteTerminalServer::Control::KeyEscape;
				}
				return -1;
			}
			return byte;
		}
		return -1;
	}
}

int RemoteTerminalServer::Terminal::readLine( uint8_t* data, uint32_t maxSize )
{
	if( server->inFile )
	{
		uint32_t size = 0;
		while( true )
		{
			if( server->terminationReqReceived )
				return -1;

			uint32_t maxPartSize = maxSize - size;
			if( maxPartSize > 64 )
				maxPartSize = 64;

			uint32_t partSize;
			if( ( partSize = server->inFile->read( data, maxPartSize ) ) == 0 )
				return size == 0 ? ( int )-1 : ( int )size;

			uint8_t* p = data;
			uint8_t* const end = data + partSize;
			while( p != end && *p != '\n' )
				++p;

			size += p - data;
			if( p == end )
			{
				if( size == maxSize )
					return size;
			}
			else if( *p == '\n' )
			{
				server->inFile->seek( server->inFile->pos() - ( end - ( p + 1 ) ) );
				return size;
			}

			data = p;
		}
	}
	else
	{
		static const auto removeSymbol = []( uint8_t* p, uint8_t* end ) {
			uint8_t* q = p + 1;
			while( q != end )
				*p++ = *q++;
		};
		static const auto insertSymbol = []( uint8_t* p, uint8_t* end, uint8_t symbol ) {
			uint8_t* q = end - 1;
			while( q != p )
				*end -- = *q--;
			*end -- = *q--;
			*p = symbol;
		};

		uint8_t* p = data;
		uint32_t size = 0;
		while( size < maxSize )
		{
			int code = readKeyCode();
			if( code == -1 )
			{
				moveCursor( data + size - p );
				return -1;
			}
			if( code == '\n' || code == RemoteTerminalServer::VKeyReturn || code == RemoteTerminalServer::VKeyEnter )
				break;
			if( code == RemoteTerminalServer::VKeyTab )
				code = '\t';
			if( code <= 255 )
			{
				uint8_t symbol = ( uint8_t )code;
				if( p == data + size )
					*p++ = symbol;
				else
					insertSymbol( p++, data + size, symbol );
				++size;
				if( symbol == 255 )
					server->sendControl( RemoteTerminalServer::Control::Char255 );
				else
					server->sendData( &symbol, 1 );
				continue;
			}
			switch( code )
			{
			case RemoteTerminalServer::VKeyLeft:
				if( p != data )
				{
					--p;
					moveCursor( -1 );
				}
				break;
			case RemoteTerminalServer::VKeyRight:
				if( p != data + size )
				{
					++p;
					moveCursor( 1 );
				}
				break;
			case RemoteTerminalServer::VKeyHome:
				if( p != data )
				{
					moveCursor( -( p - data ) );
					p = data;
				}
				break;
			case RemoteTerminalServer::VKeyEnd:
				if( p != data + size )
				{
					moveCursor( data + size - p );
					p = data + size;
				}
				break;
			case RemoteTerminalServer::VKeyBackspace:
				if( p != data )
				{
					--p;
					removeSymbol( p, data + size );
					--size;
					deletePrevSymbol();
				}
				break;
			case RemoteTerminalServer::VKeyDelete:
				if( p != data + size )
				{
					removeSymbol( p, data + size );
					--size;
					deleteNextSymbol();
				}
				break;
			default:
				break;
			}
		}
		moveCursor( data + size - p );

		return size;
	}
}

int RemoteTerminalServer::Terminal::readBytes( uint8_t* data, uint32_t maxSize )
{
	if( server->inFile )
	{
		if( server->terminationReqReceived )
			return -1;
		uint32_t size;
		if( ( size = server->inFile->read( data, maxSize ) ) )
			return size == 0 ? -1 : ( int )size;
		return -1;
	}
	else
	{
		CriticalSectionLocker locker;
		uint32_t size = maxSize;
		while( size )
		{
			if( server->terminationReqReceived )
				return -1;

			if( server->in.waitForReadAvailable( size, TIME_INFINITE ) && server->in.read( data, size ) )
			{
				uint8_t* p = data;
				uint8_t* const end = data + size;
				while( p != end && *p != 255 )
					++p;
				if( p == end )
					return maxSize;
				
				uint8_t* q = p;
				while( q != end )
				{
					if( *q == 255 )
					{
						++q;
						if( *q == RemoteTerminalServer::Control::Char255 )
							*p++ = *q++;
						else if( *q == RemoteTerminalServer::Control::KeyReturn || *q == RemoteTerminalServer::Control::KeyEnter )
							*p++ = '\n', ++q;
						else if( *q == RemoteTerminalServer::Control::KeyTab )
							*p++ = '\t', ++q;
					}
					else
						*p++ = *q++;
				}

				size -= p - data;
				data = p;
			}
			else
				return -1;
		}

		return -1;
	}
}

bool RemoteTerminalServer::Terminal::isTerminationRequested()
{
	return server->terminationReqReceived;
}

void RemoteTerminalServer::Terminal::setCursorReplaceMode( bool enabled )
{
	server->sendControl( enabled ? RemoteTerminalServer::Control::CursorReplaceModeEnable : RemoteTerminalServer::Control::CursorReplaceModeDisable );
}

void RemoteTerminalServer::Terminal::moveCursor( int n )
{
	if( n == 0 )
		return;
	if( n == 1 )
	{
		uint8_t data[2] = { 255, RemoteTerminalServer::Control::CursorMoveRight };
		server->outputCallback( data, 2, server->outputCallbackPrm );
	}
	else if( n == -1 )
	{
		uint8_t data[2] = { 255, RemoteTerminalServer::Control::CursorMoveLeft };
		server->outputCallback( data, 2, server->outputCallbackPrm );
	}
	else
	{
		const uint16_t _n = ( uint16_t )n;
		uint8_t data[4] = { 255, RemoteTerminalServer::Control::CursorMoveN, ( uint8_t )( _n >> 8 ), ( uint8_t )_n };
		server->outputCallback( data, 4, server->outputCallbackPrm );
	}
}

void RemoteTerminalServer::Terminal::moveCursorToEnd()
{
	server->sendControl( RemoteTerminalServer::Control::CursorMoveToEnd );
}

void RemoteTerminalServer::Terminal::moveCursorToBegin()
{
	server->sendControl( RemoteTerminalServer::Control::CursorMoveToBegin );
}

void RemoteTerminalServer::Terminal::moveCursorToEndOfLine()
{
	server->sendControl( RemoteTerminalServer::Control::CursorMoveToEndOfLine );
}

void RemoteTerminalServer::Terminal::moveCursoreToBeginOfLine()
{
	server->sendControl( RemoteTerminalServer::Control::CursorMoveToBeginOfLine );
}

void RemoteTerminalServer::Terminal::deleteNextSymbol()
{
	server->sendControl( RemoteTerminalServer::Control::ConsoleDelete );
}

void RemoteTerminalServer::Terminal::deletePrevSymbol()
{
	server->sendControl( RemoteTerminalServer::Control::ConsoleBackspace );
}

void RemoteTerminalServer::Terminal::clear()
{
	server->sendControl( RemoteTerminalServer::Control::ConsoleClear );
}

void RemoteTerminalServer::Terminal::setTextColor( uint8_t r, uint8_t g, uint8_t b )
{
	uint8_t data[5] = { 255, RemoteTerminalServer::Control::ConsoleSetTextColor, r, g, b };
	server->sendData( data, 5 );
}

void RemoteTerminalServer::Terminal::setBackgroundColor( uint8_t r, uint8_t g, uint8_t b )
{
	uint8_t data[5] = { 255, RemoteTerminalServer::Control::ConsoleSetBackgroundColor, r, g, b };
	server->sendData( data, 5 );
}

void RemoteTerminalServer::Terminal::restoreColors()
{
	server->sendControl( RemoteTerminalServer::Control::ConsoleRestoreColors );
}
