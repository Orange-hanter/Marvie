#pragma once

#include "RemoteTerminalServer.h"

namespace CommandLineUtility
{
	int ls( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] );
	int mkdir( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] );
	int rm( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] );
	int cat( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] );
	int ping( RemoteTerminalServer::Terminal* terminal, int argc, char* argv[] );
}