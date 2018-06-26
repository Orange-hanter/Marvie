#include "LexicalAnalyzer.h"

void LexicalAnalyzer::Parser::parsingCompleted( ParsingResult* result, const ByteRingIterator& endToken )
{
	analyzer->parsingCompleted( result, endToken );
}

void LexicalAnalyzer::Parser::dropParsing()
{
	analyzer->dropParsing();
}

void LexicalAnalyzer::Token::setName( const char* name )
{
	this->name = name;
}

void LexicalAnalyzer::Token::setParser( Parser* parser )
{
	this->parser = parser;
	parser->analyzer = analyzer;
}

void LexicalAnalyzer::Token::setParsingResultType( int type )
{
	resType = type;
}

LexicalAnalyzer::State& LexicalAnalyzer::State::addRelatedToken( Token* token )
{
	relatedTokens.push_back( token );
	return *this;
}

LexicalAnalyzer::LexicalAnalyzer( uint32_t tokensCount, uint32_t stateCount )
{
	tokens = new Token[tokensCount];
	for( uint32_t i = 0; i < tokensCount; i++ )
	{
		tokens[i].id = i;
		tokens[i].analyzer = this;
	}
	states = new State[stateCount + 1];
	for( uint32_t i = 0; i < stateCount + 1; i++ )
		states[i].id = i;
	currentState = states;
	universumEnable = true;
	buffer = nullptr;
	activeParser = nullptr;
}

LexicalAnalyzer::~LexicalAnalyzer()
{
	delete[] tokens;
	delete[] states;
}

LexicalAnalyzer::Token* LexicalAnalyzer::token( uint32_t id )
{
	return tokens + id;
}

LexicalAnalyzer::State* LexicalAnalyzer::state( uint32_t id )
{
	return states + id;
}

void LexicalAnalyzer::setBuffer( AbstractReadable* buffer )
{
	this->buffer = buffer;
	current = buffer->begin();
}

void LexicalAnalyzer::setCallback( std::function< void( ParsingResult* ) > callBack )
{
	this->callBack = callBack;
}

void LexicalAnalyzer::setState( uint32_t id )
{
	if( id != currentState->id && id != 0 )
		resetState( id );
	currentState = states + id;
}

void LexicalAnalyzer::setUniversumStateEnabled( bool enable )
{
	if( universumEnable == enable )
		return;
	universumEnable = enable;
	if( enable )
		resetState( 0 );
}

bool LexicalAnalyzer::isUniversumStateEnabled()
{
	return universumEnable;
}

void LexicalAnalyzer::processNewBytes()
{
	ByteRingIterator end;
	bool res;
Start:
	end = buffer->end();
	if( buffer->isOverflowed() )
		return;
	res = false;
	if( activeParser )
		res = activeParser->parse( current, end );
	if( res && activeParser )
		goto Start;

	end = buffer->end();
	if( buffer->isOverflowed() )
		return;
	if( !activeParser && current != end )
	{
		State* ps[2];
		int sCount = 0;
		if( universumEnable )
			ps[0] = states, ++sCount;
		if( currentState->id != 0 )
			ps[sCount] = currentState, ++sCount;

		for( auto iByte = current; iByte != end; iByte++ )
		{
			for( int iState = 0; iState < sCount; iState++ )
			{
				for( auto iToken = ps[iState]->relatedTokens.begin(); iToken != ps[iState]->relatedTokens.end(); iToken++ )
				{
					if( ( *iToken )->name[( *iToken )->pos] == *iByte )
					{
Ok:
						if( ( *iToken )->name[++( *iToken )->pos] == 0 )
						{
							resetState( 0 );
							if( currentState->id != 0 )
								resetState( currentState->id );

							if( ( *iToken )->parser )
							{
								current = iByte + 1;
								activeParser = ( *iToken )->parser;
							}
							else
							{
								ParsingResult res( ( *iToken )->resType );
								parsingCompleted( &res, iByte + 1 );
							}
							goto Start;
						}
					}
					else if( ( *iToken )->pos )
					{
						( *iToken )->pos = 0;
						if( ( *iToken )->name[0] == *iByte )
							goto Ok;
					}
						
				}
			}
		}
		current = end;
	}	
}

void LexicalAnalyzer::reset()
{
	current = buffer->begin();
	if( activeParser )
	{
		activeParser->reset();
		activeParser = nullptr;
	}
	if( universumEnable )
		resetState( 0 );
	if( currentState->id != 0 )
		resetState( currentState->id );
}

void LexicalAnalyzer::parsingCompleted( ParsingResult* result, const ByteRingIterator& endToken )
{
	buffer->read( nullptr, endToken - buffer->begin() );
	current = endToken;
	activeParser = nullptr;
	callBack( result );
}

void LexicalAnalyzer::dropParsing()
{
	activeParser = nullptr;
}

void LexicalAnalyzer::resetState( uint32_t id )
{
	State* s = states + id;
	for( auto i = s->relatedTokens.begin(); i != s->relatedTokens.end(); i++ )
		( *i )->pos = 0;
}