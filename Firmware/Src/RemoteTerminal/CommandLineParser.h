#pragma once

#include <vector>
#include <string>

class CommandLineParser
{
public:
	enum class StdIn { Keyboard, File, Pipeline };
	enum class StdOut { Terminal, File, FileAppend, Pipeline };
	enum class StdErr { Terminal, File, FileAppend, StdOut };
	enum class LaunchCondition { Unconditional, PrevSucceed, PrevFail };

	struct Command
	{
		Command() noexcept;
		Command( const Command& other ) = delete;
		Command( Command&& other ) noexcept;
		~Command();

		Command& operator=( const Command& other ) = delete;
		Command& operator=( Command&& other ) = delete;

		inline const char* name() const noexcept { return _name; }
		inline StdIn stdIn() const noexcept { return _in; }
		inline StdOut stdOut() const noexcept { return _out; }
		inline StdErr stdErr() const noexcept { return _err; }
		inline const char* inFileName() const noexcept { return _inFileName; }
		inline const char* outFileName() const noexcept { return _outFileName; }
		inline const char* errFileName() const noexcept { return _errFileName; }
		inline LaunchCondition launchCondition() const noexcept { return _launch; }
		inline int argc() const noexcept { return _argc; }
		inline char** const argv() const noexcept { return _argv; }

	private:
		friend class CommandLineParser;
		char* _name;
		StdIn _in;
		StdOut _out;
		StdErr _err;
		char* _inFileName;
		char* _outFileName;
		char* _errFileName;
		LaunchCondition _launch;
		int _argc;
		char** _argv;
	};

	CommandLineParser() noexcept;
	CommandLineParser( const CommandLineParser& other ) = delete;
	CommandLineParser( CommandLineParser&& other ) = default;
	~CommandLineParser();

	bool parse( const char* line );
	void clear() noexcept;

	const char* errorString() const noexcept;
	int errorPosition() const noexcept;
	int commandCount() const noexcept;
	const Command* command( int index ) const noexcept;

private:
	const char* nextLexeme( const char* begin, const char* end );
	int symbolLevel( char c ) const noexcept;
	bool isControlSymbol( char c ) const noexcept;
	bool containsControlSymbol( const std::string& str ) const noexcept;
	enum class Redirection { Unknown, In, Out, OutAppend, Err, ErrAppend };
	Redirection toRedirection( const std::string& str ) const noexcept;
	const char* skipSpaces( const char* str ) const noexcept;
	char* copyString( const std::string& str ) const;
	bool isValidNameSymbol( char c ) const noexcept;
	bool isValidCommandName( const std::string& name ) const noexcept;

private:
	std::vector< Command > commands;
	std::string lex;
	volatile bool lexEsc = false;
	const char* errStr;
	int errPos;
};