#include "CommandLineUtility.h"
#include "FileSystem/FileInfoReader.h"
#include "Network/PingService.h"
#include "Support/Utility.h"
#include <string.h>

int CommandLineUtility::ls( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] )
{
	std::string dirName;
	for( int i = 0; i < argc; ++i )
	{
		if( argv[i][0] != '-' )
			dirName = argv[i];
	}

	const char header[] = " Attr         Last Modified          Size   Name\n"
						  "-----   -------------------   -----------   ------";
						//"drash   31.12.2019 23:59:42   17179869184   gg.txt"

	FileInfoReader reader;
	reader.setDir( Dir::cleanPath( dirName ) );
	if( !reader.next() )
	{
		if( reader.lastError() == FileInfoReader::Error::NoError )
		{
			terminal->stdOutWrite( ( uint8_t* )header, sizeof( header ) - 1 );
			return 0;
		}
		else
		{
			const char* errMsg = reader.lastErrorString();
			terminal->stdErrWrite( ( uint8_t* )errMsg, strlen( errMsg ) );
			return -1;
		}
	}

	terminal->stdOutWrite( ( uint8_t* )header, sizeof( header ) - 1 );
	do
	{
		char str[100];
		char* dateTimeStr = str + 40;
		char* sizeStr = str + 70;
		auto& info = reader.info();
		str[0] = '\n';
		str[1] = info.attributes().testFlag( FileSystem::DirectoryAttr ) ? 'd' : '-';
		str[2] = info.attributes().testFlag( FileSystem::ReadOnlyAttr ) ? 'r' : '-';
		str[3] = info.attributes().testFlag( FileSystem::ArchiveAttr ) ? 'a' : '-';
		str[4] = info.attributes().testFlag( FileSystem::SystemAttr ) ? 's' : '-';
		str[5] = info.attributes().testFlag( FileSystem::HiddenAttr ) ? 'h' : '-';
		info.lastModified().printDateTime( dateTimeStr )[0] = 0;
		if( info.attributes().testFlag( FileSystem::DirectoryAttr ) )
			sizeStr[0] = 0;
		else
			Utility::printIntegral( sizeStr, info.fileSize() )[0] = 0;
		sprintf( str + 6, "   %19s   %11s   %s", dateTimeStr, sizeStr, info.fileName().c_str() );
		if( terminal->stdOutWrite( ( uint8_t* )str, strlen( str ) ) == -1 )
			break;
	} while( reader.next() );

	if( reader.lastError() != FileInfoReader::Error::NoError )
	{
		const char* errMsg = reader.lastErrorString();
		terminal->stdErrWrite( ( uint8_t* )errMsg, strlen( errMsg ) );
		return -1;
	}

	return 0;
}

int CommandLineUtility::mkdir( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] )
{
	Dir dir;
	for( int i = 0; i < argc; ++i )
	{
		if( argv[i][0] != '-' )
			dir.setPath( argv[i] );
	}

	if( !dir.mkpath() )
	{
		const char* errMsg = dir.lastErrorString();
		terminal->stdErrWrite( ( uint8_t* )errMsg, strlen( errMsg ) );
		return -1;
	}
	return 0;
}

int CommandLineUtility::rm( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] )
{
	Dir dir;
	volatile bool rOption = false;
	for( int i = 0; i < argc; ++i )
	{
		if( argv[i][0] == '-' )
		{
			if( strcmp( argv[i], "-r" ) == 0 )
				rOption = true;
			else
			{
				std::string err( "Invalid argument " );
				err.append( argv[i] );
				terminal->stdErrWrite( ( uint8_t* )err.c_str(), err.size() );
				return -1;
			}
		}
		else
			dir.setPath( argv[i] );
	}

	if( rOption )
		dir.removeRecursively();
	else
		dir.rmpath();
	if( dir.lastError() != Dir::Error::NoError )
	{
		const char* errMsg = dir.lastErrorString();
		terminal->stdErrWrite( ( uint8_t* )errMsg, strlen( errMsg ) );
		return -1;
	}
	return 0;
}

int CommandLineUtility::mv( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] )
{
	if( argc != 2 )
	{
		const char errMsg[] = "invalid arguments";
		terminal->stdErrWrite( ( uint8_t* )errMsg, sizeof( errMsg ) - 1 );
		return -1;
	}

	Dir dir;
	if( !dir.rename( argv[0], argv[1] ) )
	{
		const char* errMsg = dir.lastErrorString();
		terminal->stdErrWrite( ( uint8_t* )errMsg, strlen( errMsg ) );
		return -1;
	}

	return 0;
}

int CommandLineUtility::cat( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] )
{
	struct Resources
	{
		uint8_t buffer[512];
		File file;
	};
	std::unique_ptr< Resources > resources( new Resources );
	if( resources == nullptr )
	{
		const char errMsg[] = "Memory error";
		terminal->stdErrWrite( ( uint8_t* )errMsg, sizeof( errMsg ) - 1 );
		return -1;
	}

	for( int i = 0; i < argc; ++i )
	{
		resources->file.setFileName( argv[0] );
		if( !resources->file.open( FileSystem::OpenModeFlag::OpenExisting | FileSystem::OpenModeFlag::Read ) )
		{
			const char* errMsg = resources->file.lastErrorString();
			terminal->stdErrWrite( ( uint8_t* )errMsg, strlen( errMsg ) );
			return -1;
		}
		uint32_t size;
		while( ( size = resources->file.read( resources->buffer, 512 ) ) )
		{
			if( terminal->stdOutWrite( resources->buffer, size ) == -1 )
				return -1;
		}
		resources->file.close();
	}

	return 0;
}

int CommandLineUtility::ping( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] )
{
	IpAddress addr;
	volatile bool continuousFlag = false;
	constexpr char invalidArgsError[] = "invalid arguments";
	constexpr char invalidAddressError[] = "invalid IP address";
	constexpr char timeoutMessage[] = "\ntimeout";
	char* ipStr = nullptr;
	if( argc == 0 || argc > 2 )
	{
		terminal->stdErrWrite( ( uint8_t* )invalidArgsError, sizeof( invalidArgsError ) - 1 );
		return -1;
	}
	for( int i = 0; i < argc; ++i )
	{
		if( strcmp( argv[i], "-t" ) == 0 )
			continuousFlag = true;
		else
			addr = IpAddress( argv[i] ), ipStr = argv[i];
	}
	if( addr == IpAddress() )
	{
		terminal->stdErrWrite( ( uint8_t* )invalidAddressError, sizeof( invalidAddressError ) - 1 );
		return -1;
	}
	std::string str( "\nPinging " );
	str.append( ipStr ).append( ":" );
	terminal->stdOutWrite( ( const uint8_t* )str.c_str(), str.size() );
	int sent = 0, received = 0;
	PingService::TimeMeasurement gtm { -1, -1, -1 };
	while( true )
	{
		if( terminal->isTerminationRequested() )
			return -1;
		++sent;
		PingService::TimeMeasurement tm;
		int nOk = PingService::instance()->ping( addr, 1, &tm );
		if( nOk )
		{
			++received;
			if( sent == 1 )
				gtm = tm;
			else
			{
				if( tm.minMs < gtm.minMs )
					gtm.minMs = tm.minMs;
				if( tm.maxMs > gtm.maxMs )
					gtm.maxMs = tm.maxMs;
				gtm.avgMs += tm.avgMs;
			}
			str = "\nReply from ";
			str.append( ipStr ).append( ": time = " ).append( std::to_string( tm.avgMs ) ).append( "ms" );
			terminal->stdOutWrite( ( const uint8_t* )str.c_str(), str.size() );
		}
		else
			terminal->stdOutWrite( ( uint8_t* )timeoutMessage, sizeof( timeoutMessage ) - 1 );
		if( !continuousFlag && sent == 4 )
			break;
	}
	if( !continuousFlag )
	{
		str = "\n\nPing statistics for ";
		str.append( ipStr ).append( ":\n    Packets: Sent = " ).append( std::to_string( sent ) ).
		append( ", Received = " ).append( std::to_string( received ) ).
		append( ", Lost = ").append( std::to_string( sent - received ) ).
		append( " (" ).append( std::to_string( ( sent - received ) * 100 / sent ) ).append( "%)" );
		if( received )
		{
			str.append( ",\nApproximate round trip times in milli-seconds:\n    " ).
			append( "Minimum = " ).append( std::to_string( gtm.minMs ) ).append( "ms, " ).
			append( "Maximum = " ).append( std::to_string( gtm.maxMs ) ).append( "ms, " ).
			append( "Average = " ).append( std::to_string( gtm.avgMs / sent ) ).append( "ms\n" );
		}
		else
			str.append( "\n" );
		terminal->stdOutWrite( ( const uint8_t* )str.c_str(), str.size() );
	}

	return 0;
}
