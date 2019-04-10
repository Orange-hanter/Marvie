#include "CommandLineParser.h"
#include <list>
#include <memory>
#include <string.h>

CommandLineParser::Command::Command() noexcept
{
	_name = _inFileName = _outFileName = _errFileName = nullptr;
	_in = StdIn::Keyboard;
	_out = StdOut::Terminal;
	_err = StdErr::Terminal;
	_launch = LaunchCondition::Unconditional;
	_argc = 0;
	_argv = nullptr;
}

CommandLineParser::Command::Command( Command&& other ) noexcept
{
	_name = other._name;
	_in = other._in;
	_out = other._out;
	_err = other._err;
	_inFileName = other._inFileName;
	_outFileName = other._outFileName;
	_errFileName = other._errFileName;
	_launch = other._launch;
	_argc = other._argc;
	_argv = other._argv;
	other._name = nullptr;
	other._in = StdIn::Keyboard;
	other._out = StdOut::Terminal;
	other._err = StdErr::Terminal;
	other._inFileName = nullptr;
	other._outFileName = nullptr;
	other._errFileName = nullptr;
	other._launch = LaunchCondition::Unconditional;
	other._argc = 0;
	other._argv = nullptr;
}

CommandLineParser::Command::~Command()
{
	delete _name;
	delete _inFileName;
	delete _outFileName;
	delete _errFileName;
	for( int i = 0; i < _argc; ++i )
		delete _argv[i];
	delete _argv;
}

CommandLineParser::CommandLineParser() noexcept
{
	errStr = nullptr;
	errPos = -1;
}

CommandLineParser::~CommandLineParser()
{
	clear();
}

bool CommandLineParser::parse( const char* line )
{
	clear();
	std::list< Command > list;
	LaunchCondition nextLaunchCondition = LaunchCondition::Unconditional;
	StdIn nextStdIn = StdIn::Keyboard;

	const char* lineEnd = line + strlen( line );
	const char* p = line;
	const char* next;
	while( p != lineEnd )
	{
		Command com;
		com._in = nextStdIn;
		com._launch = nextLaunchCondition;
		std::list< std::unique_ptr< char > > args;

		next = nextLexeme( p, lineEnd );
		if( errStr )
		{
			errPos = ( int )( next - line );
			return false;
		}
		if( lex.empty() || lexEsc || !isValidCommandName( lex ) )
		{
			errStr = "Invalid command name";
			errPos = ( int )( skipSpaces( p ) - line );
			return false;
		}
		com._name = copyString( lex );
		if( com._name == nullptr )
		{
			errStr = "Memory error";
			return false;
		}

		volatile bool parsingArgs = true;
		while( true )
		{
			p = next;
			next = nextLexeme( p, lineEnd );
			if( errStr )
			{
				errPos = ( int )( next - line );
				return false;
			}

			if( lex.empty() || !lexEsc && ( lex == "&" || lex == "|" || lex == "&&" || lex == "||" ) )
				break;
			Redirection redir;
			if( !lexEsc && ( redir = toRedirection( lex ) ) != Redirection::Unknown )
			{
				if( redir == Redirection::In ? com._in != StdIn::Keyboard : redir == Redirection::Out
					|| redir == Redirection::OutAppend ? com._out != StdOut::Terminal : com._err != StdErr::Terminal )
				{
					errStr = "Double redirection";
					errPos = ( int )( skipSpaces( p ) - line );
					return false;
				}

				parsingArgs = false;
				p = next;
				next = nextLexeme( p, lineEnd );
				if( errStr )
				{
					errPos = ( int )( next - line );
					return false;
				}

				if( lex.empty() )
				{
					errStr = "Syntax error";
					errPos = ( int )( p - line );
					return false;
				}
				if( !lexEsc )
				{
					if( ( redir == Redirection::Err || redir == Redirection::ErrAppend ) && lex == "&1" )
					{
						if( com._out == StdOut::Terminal )
						{
							errStr = "Syntax error";
							errPos = ( int )( skipSpaces( p ) - line );
							return false;
						}
						com._err = StdErr::StdOut;
					}
					else
					{
						if( toRedirection( lex ) != Redirection::Unknown || lex == "&" || lex == "|" || lex == "&&" || lex == "||" )
						{
							errStr = "Syntax error";
							errPos = ( int )( skipSpaces( p ) - line );
							return false;
						}
						goto Process;
					}
				}
				else
				{
				Process:
					switch( redir )
					{
					case CommandLineParser::Redirection::In:
						com._in = StdIn::File;
						com._inFileName = copyString( lex );
						if( com._inFileName == nullptr )
						{
							errStr = "Memory error";
							return false;
						}
						break;
					case CommandLineParser::Redirection::Out:
						com._out = StdOut::File;
						com._outFileName = copyString( lex );
						if( com._outFileName == nullptr )
						{
							errStr = "Memory error";
							return false;
						}
						break;
					case CommandLineParser::Redirection::OutAppend:
						com._out = StdOut::FileAppend;
						com._outFileName = copyString( lex );
						if( com._outFileName == nullptr )
						{
							errStr = "Memory error";
							return false;
						}
						break;
					case CommandLineParser::Redirection::Err:
						com._err = StdErr::File;
						com._errFileName = copyString( lex );
						if( com._errFileName == nullptr )
						{
							errStr = "Memory error";
							return false;
						}
						break;
					case CommandLineParser::Redirection::ErrAppend:
						com._err = StdErr::FileAppend;
						com._errFileName = copyString( lex );
						if( com._errFileName == nullptr )
						{
							errStr = "Memory error";
							return false;
						}
						break;
					default:
						break;
					}
				}
			}
			else
			{
				if( !parsingArgs )
				{
					errStr = "Syntax error";
					errPos = ( int )( skipSpaces( p ) - line );
					return false;
				}
				char* str = copyString( lex );
				if( str == nullptr )
				{
					errStr = "Memory error";
					return false;
				}
				args.push_back( std::unique_ptr< char >( str ) );
			}
		}

		if( args.size() )
		{
			com._argv = new char*[args.size()];
			if( com._argv == nullptr )
			{
				errStr = "Memory error";
				return false;
			}
			for( auto i = args.begin(); i != args.end(); ++i )
				com._argv[com._argc++] = ( *i ).release();
		}

		list.push_back( std::move( com ) );
		if( lex.empty() )
			break;

		nextStdIn = StdIn::Keyboard;
		if( lex == "&" )
			nextLaunchCondition = LaunchCondition::Unconditional;
		else if( lex == "&&" )
			nextLaunchCondition = LaunchCondition::PrevSucceed;
		else if( lex == "|" )
		{
			if( list.back().stdOut() != StdOut::Terminal )
			{
				errStr = "Double redirection";
				errPos = ( int )( skipSpaces( p ) - line );
				return false;
			}
			else
				list.back()._out = StdOut::Pipeline;
			nextLaunchCondition = LaunchCondition::Unconditional;
			nextStdIn = StdIn::Pipeline;
		}
		else if( lex == "||" )
			nextLaunchCondition = LaunchCondition::PrevFail;
		p = next;
	}

	if( list.size() )
	{
		commands.reserve( list.size() );
		for( auto i = list.begin(); i != list.end(); ++i )
			commands.push_back( std::move( *i ) );
	}

	return true;
}

void CommandLineParser::clear() noexcept
{
	commands.clear();
	errStr = nullptr;
	errPos = -1;
}

const char* CommandLineParser::errorString() const noexcept
{
	return errStr;
}

int CommandLineParser::errorPosition() const noexcept
{
	return errPos;
}

int CommandLineParser::commandCount() const noexcept
{
	return ( int )commands.size();
}

const CommandLineParser::Command* CommandLineParser::command( int index ) const noexcept
{
	return &commands.at( index );
}

const char* CommandLineParser::nextLexeme( const char* begin, const char* end )
{
	lex.clear();
	lexEsc = false;
	volatile bool esc = false;
	const char* p = begin;
	for( ; p != end; ++p )
	{
		if( !esc && *p == ' ' )
		{
			if( !lex.empty() )
				break;
			continue;
		}
		if( lex.size() && symbolLevel( lex.back() ) != symbolLevel( *p )
			&& !( lex.back() == '2' && *p == '>' && lex.size() == 1 )
			&& !( lex.back() == '&' && *p == '1' && lex.size() == 1 ) )
			break;
		if( *p == '\\' && p + 1 != end && p[1] == '\'' )
		{
			lex.push_back( p[1] );
			++p;
			continue;
		}
		if( *p == '\'' )
		{
			if( !lex.empty() && !esc )
				break;
			if( esc )
			{
				++p;
				esc = false;
				break;
			}
			esc = true;
			lexEsc = true;
		}
		else
			lex.push_back( *p );
	}
	if( esc )
		errStr = "Unexpected end of line";
	else if( lex.size() > 2 && ( lex.front() == '&' || lex.front() == '|' || lex.front() == '>' ) )
		errStr = "Syntax error", p -= lex.size();
	else if( lex.size() > 1 && lex.front() == '<' )
		errStr = "Syntax error", p -= lex.size();

	return p;
}

int CommandLineParser::symbolLevel( char c ) const noexcept
{
	if( c == '<' )
		return 1;
	if( c == '>' )
		return 2;
	if( c == '|' )
		return 3;
	if( c == '&' )
		return 4;
	return 0;
}

bool CommandLineParser::isControlSymbol( char c ) const noexcept
{
	return c == '>' || c == '<' || c == '|' || c == '&';
}

bool CommandLineParser::containsControlSymbol( const std::string& str ) const noexcept
{
	for( auto i = str.begin(); i != str.end(); ++i )
	{
		if( isControlSymbol( *i ) )
			return true;
	}
	return false;
}

CommandLineParser::Redirection CommandLineParser::toRedirection( const std::string& str ) const noexcept
{
	if( str == "<" )
		return Redirection::In;
	if( str == ">" )
		return Redirection::Out;
	if( str == ">>" )
		return Redirection::OutAppend;
	if( str == "2>" )
		return Redirection::Err;
	if( str == "2>>" )
		return Redirection::ErrAppend;
	return Redirection::Unknown;
}

const char* CommandLineParser::skipSpaces( const char* str ) const noexcept
{
	while( *str == ' ' )
		++str;
	return str;
}

char* CommandLineParser::copyString( const std::string& str ) const
{
	char* s = new char[str.size() + 1];
	if( s == nullptr )
		return nullptr;
	str.copy( s, str.size(), 0 );
	s[str.size()] = 0;
	return s;
}

bool CommandLineParser::isValidNameSymbol( char c ) const noexcept
{
	return ( c >= 'A' && c <= 'Z' ) || ( c >= 'a' && c <= 'z' ) || ( c >= '0' && c <= '9' ) || c == '_' || c == '.';
}

bool CommandLineParser::isValidCommandName( const std::string& name ) const noexcept
{
	for( auto i = name.begin(); i != name.end(); ++i )
	{
		if( !isValidNameSymbol( *i ) )
			return false;
	}

	return true;
}
