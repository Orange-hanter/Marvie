#pragma once

#include "Core/AbstractReadable.h"
#include <functional>
#include <list>

class LexicalAnalyzer
{
public:
	class ParsingResult
	{
	public:
		ParsingResult( int type ) : t( type ) {}
		inline int type() { return t; }

	private:
		int t;
	};

	class Parser
	{
		friend class LexicalAnalyzer;

	public:
		Parser() { analyzer = nullptr; }

	protected:
		virtual bool parse( const ByteRingIterator& begin, const ByteRingIterator& end ) = 0;
		virtual void reset() {};
		void parsingCompleted( ParsingResult* result, const ByteRingIterator& endToken );
		void dropParsing();

	private:
		LexicalAnalyzer* analyzer;
	};

	class Token
	{
		friend class LexicalAnalyzer;
		Token() { name = nullptr; parser = nullptr; resType = 0; pos = id = 0; analyzer = nullptr; }
		~Token() {};

	public:
		void setName( const char* name );
		void setParser( Parser* parser );
		void setParsingResultType( int type );
		inline uint32_t index() { return id; }

	private:
		const char* name;
		Parser* parser;
		int resType;
		uint32_t pos;
		uint32_t id;
		LexicalAnalyzer* analyzer;
	};

	class State
	{
		friend class LexicalAnalyzer;
		State() { id = 0; }

	public:
		State& addRelatedToken( Token* );
		inline uint32_t index() { return id; }

	private:
		std::list< Token* > relatedTokens;
		uint32_t id;
	};

	LexicalAnalyzer( uint32_t tokensCount, uint32_t stateCount ); // ”ниверсум не считаетс€
	~LexicalAnalyzer();

	Token* token( uint32_t id );	
	State* state( uint32_t id ); // if id == 0 then method will return the universum

	void setBuffer( AbstractReadable* buffer );
	void setCallback( std::function< void( ParsingResult* ) > callBack );
	void setState( uint32_t id ); // if id == 0 then state will be reset to universum
	void setUniversumStateEnabled( bool enable );
	bool isUniversumStateEnabled();

	void processNewBytes();
	void reset();

private:
	void parsingCompleted( ParsingResult* result, const ByteRingIterator& endToken );
	void dropParsing();
	void resetState( uint32_t id );

private:
	Token* tokens;
	State* states, *currentState;
	bool universumEnable;
	AbstractReadable* buffer;
	std::function< void( ParsingResult* ) > callBack;
	ByteRingIterator current;
	Parser* activeParser;
};