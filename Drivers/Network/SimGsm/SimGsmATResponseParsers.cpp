#include "SimGsmATResponseParsers.h"
#include <string.h>

bool SimGsmATResponseParsers::CPinParser::parse( const ByteRingIterator& begin, const ByteRingIterator& end )
{
	if( state == State::WaitingHeader )
	{
		i = begin;
		state = State::WaitingEndSeq;
	}

	while( i != end )
	{
		if( *( i - 1 ) == '\r' && *i == '\n' )
		{
			auto endText = i - 1;
			if( ( endText - begin ) - 2 > 20 )
				res.status = CPinParsingResult::Status::Unknown;
			else
			{
				int q = 0;
				for( auto g = begin + 2; g != endText; ++g )
					str[q++] = *g;
				str[q] = 0;

				if( strcmp( str, "READY" ) == 0 )
					res.status = CPinParsingResult::Status::Ready;
				else if( strcmp( str, "SIM PIN" ) == 0 )
					res.status = CPinParsingResult::Status::SimPin;
				else if( strcmp( str, "NOT INSERTED" ) == 0 )
					res.status = CPinParsingResult::Status::NotInserted;
				else if( strcmp( str, "SIM PUK" ) == 0 )
					res.status = CPinParsingResult::Status::SimPuk;
				else if( strcmp( str, "PH_SIM PIN" ) == 0 )
					res.status = CPinParsingResult::Status::PhSimPin;
				else if( strcmp( str, "PH_SIM PUK" ) == 0 )
					res.status = CPinParsingResult::Status::PhSimPuk;
				else if( strcmp( str, "SIM PIN2" ) == 0 )
					res.status = CPinParsingResult::Status::SimPin2;
				else if( strcmp( str, "SIM PUK2" ) == 0 )
					res.status = CPinParsingResult::Status::SimPuk2;
				else
					res.status = CPinParsingResult::Status::Unknown;
			}

			state = State::WaitingHeader;
			parsingCompleted( &res, i + 1 );
			return true;
		}
		++i;
	}

	return false;
}

void SimGsmATResponseParsers::CPinParser::reset()
{
	state = State::WaitingHeader;
}

bool SimGsmATResponseParsers::ReceiveParser::parse( const ByteRingIterator& begin, const ByteRingIterator& end )
{
	if( state == State::Unknown )
	{
		i = begin;
		dataSize = 0;
		state = State::WaitingHeader;
	}

	if( state == State::WaitingHeader )
	{
		while( i != end )
		{
			if( *i == '\n' )
			{
				i = begin;
				++i; res.linkId = parseInt();
				++i; dataSize = parseInt();
				++i; res.addr[3] = parseInt();
				++i; res.addr[2] = parseInt();
				++i; res.addr[1] = parseInt();
				++i; res.addr[0] = parseInt();
				++i; res.port = parseInt();
				++i; ++i;
				state = State::WaitingData;
				break;
			}
			++i;
		}

	}

	if( state == State::WaitingData )
	{
		if( end - i >= dataSize )
		{
			res.begin = i;
			res.size = dataSize;
			state = State::Unknown;

			parsingCompleted( &res, res.begin + res.size );
			return true;
		}
	}

	return false;
}

void SimGsmATResponseParsers::ReceiveParser::reset()
{
	state = State::Unknown;
	dataSize = 0;
}

int SimGsmATResponseParsers::ReceiveParser::parseInt()
{
	int v = 0;
	while( *i >= '0' && *i <= '9' )
	{
		v *= 10;
		v += *i++ - '0';
	}

	return v;
}

void SimGsmATResponseParsers::parseIp( const ByteRingIterator& begin, const ByteRingIterator& end, uint8_t addr[4] )
{
	int index = 0;
	ByteRingIterator i = begin;
	uint8_t tmp[4] = {};
	while( i != end )
	{
		if( *i >= '0' && *i <= '9' )
		{
			tmp[index] *= 10;
			tmp[index] += *i - '0';
		}
		else
			++index;
		++i;
	}
	addr[0] = tmp[3];
	addr[1] = tmp[2];
	addr[2] = tmp[1];
	addr[3] = tmp[0];
}

bool SimGsmATResponseParsers::RemoteIpParser::parse( const ByteRingIterator& begin, const ByteRingIterator& end )
{
	if( state == State::WaitingHeader )
	{
		i = begin;
		state = State::WaitingEndSeq;
	}

	while( i != end )
	{
		if( *( i - 1 ) == '\r' && *i == '\n' )
		{
			res.linkId = *( begin - 12 ) - '0';
			parseIp( begin + 2, i - 1, res.addr );
			state = State::WaitingHeader;

			parsingCompleted( &res, i + 1 );
			return true;
		}
		++i;
	}

	return false;
}

void SimGsmATResponseParsers::RemoteIpParser::reset()
{
	state = State::WaitingHeader;
}

bool SimGsmATResponseParsers::IpParser::parse( const ByteRingIterator& begin, const ByteRingIterator& end )
{
	if( state == State::WaitingHeader )
	{
		if( end - begin > 0 )
		{
			if( *begin < '0' || *begin > '9' )
			{
				dropParsing();
				return false;
			}
		}
		else
			return false;
		i = begin;
		state = State::WaitingEndSeq;
	}

	while( i != end )
	{
		if( *( i - 1 ) == '\r' && *i == '\n' )
		{
			parseIp( begin, i - 1, res.addr );
			state = State::WaitingHeader;

			parsingCompleted( &res, i + 1 );
			return true;
		}
		++i;
	}

	return false;
}

void SimGsmATResponseParsers::IpParser::reset()
{
	state = State::WaitingHeader;
}

bool SimGsmATResponseParsers::ClosedParser::parse( const ByteRingIterator& begin, const ByteRingIterator& end )
{
	res.linkId = *( begin - 11 ) - '0';
	parsingCompleted( &res, begin );
	return true;
}

bool SimGsmATResponseParsers::CloseOkParser::parse( const ByteRingIterator& begin, const ByteRingIterator& end )
{
	res.linkId = *( begin - 13 ) - '0';
	parsingCompleted( &res, begin );
	return true;
}

bool SimGsmATResponseParsers::AlreadyConnectParser::parse( const ByteRingIterator& begin, const ByteRingIterator& end )
{
	res.linkId = *( begin - 20 ) - '0';
	parsingCompleted( &res, begin );
	return true;
}

bool SimGsmATResponseParsers::ConnectOkParser::parse( const ByteRingIterator& begin, const ByteRingIterator& end )
{
	res.linkId = *( begin - 15 ) - '0';
	parsingCompleted( &res, begin );
	return true;
}

bool SimGsmATResponseParsers::ConnectFailParser::parse( const ByteRingIterator& begin, const ByteRingIterator& end )
{
	res.linkId = *( begin - 17 ) - '0';
	parsingCompleted( &res, begin );
	return true;
}

bool SimGsmATResponseParsers::CMEErrorParser::parse( const ByteRingIterator& begin, const ByteRingIterator& end )
{
	if( state == State::WaitingHeader )
	{
		i = begin;
		res.code = 0;
		state = State::WaitingEndSeq;
	}

	while( i != end )
	{
		if( *( i - 1 ) == '\r' && *i == '\n' )
		{
			ByteRingIterator iCode = begin + 2;
			while( *iCode >= '0' && *iCode <= '9' )
			{
				res.code *= 10;
				res.code += *iCode++ - '0';
			}
			state = State::WaitingHeader;

			parsingCompleted( &res, i + 1 );
			return true;
		}
		++i;
	}

	return false;
}

void SimGsmATResponseParsers::CMEErrorParser::reset()
{
	state = State::WaitingHeader;
}