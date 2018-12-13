#pragma once

#include "Support/LexicalAnalyzer.h"

namespace SimGsmATResponseParsers
{
	enum ParserResultType { CPin, CallReady, SmsReady, PdpDeact, Receive, RemoteIp, Ip, Closed, Ok, ServerOk, ServerClose, SendInitOk, SendOk, SendFail, CloseOk, AlreadyConnect, ConnectOk, ConnectFail, Error, CMEError, PwrDown, Connect };

	class CPinParsingResult : public LexicalAnalyzer::ParsingResult
	{
	public:
		CPinParsingResult() : ParsingResult( ParserResultType::CPin ) { status = Status::Ready; }

		enum class Status { NotInserted, Ready, SimPin, SimPuk, PhSimPin, PhSimPuk, SimPin2, SimPuk2, Unknown } status;
	};

	class ReceiveParsingResult : public LexicalAnalyzer::ParsingResult
	{
	public:
		ReceiveParsingResult() : ParsingResult( ParserResultType::Receive ) { linkId = 0; addr[0] = addr[1] = addr[2] = addr[3] = 0; port = 0; size = 0; }

		int linkId;
		uint8_t addr[4];
		uint16_t port;
		uint16_t size;
		ByteRingIterator begin;
	};

	class RemoteIpParsingResult : public LexicalAnalyzer::ParsingResult
	{
	public:
		RemoteIpParsingResult() : ParsingResult( ParserResultType::RemoteIp ) { linkId = 0; addr[0] = addr[1] = addr[2] = addr[3] = 0; }

		int linkId;
		uint8_t addr[4];
	};

	class IpParsingResult : public LexicalAnalyzer::ParsingResult
	{
	public:
		IpParsingResult() : ParsingResult( ParserResultType::Ip ) { addr[0] = addr[1] = addr[2] = addr[3] = 0; }

		uint8_t addr[4];
	};

	class ClosedParsingResult : public LexicalAnalyzer::ParsingResult
	{
	public:
		ClosedParsingResult() : ParsingResult( ParserResultType::Closed ) { linkId = 0; }

		int linkId;
	};

	class CloseOkParsingResult : public LexicalAnalyzer::ParsingResult
	{
	public:
		CloseOkParsingResult() : ParsingResult( ParserResultType::CloseOk ) { linkId = 0; }

		int linkId;
	};

	class AlreadyConnectParsingResult : public LexicalAnalyzer::ParsingResult
	{
	public:
		AlreadyConnectParsingResult() : ParsingResult( ParserResultType::AlreadyConnect ) { linkId = 0; }

		int linkId;
	};

	class ConnectOkParsingResult : public LexicalAnalyzer::ParsingResult
	{
	public:
		ConnectOkParsingResult() : ParsingResult( ParserResultType::ConnectOk ) { linkId = 0; }

		int linkId;
	};

	class ConnectFailParsingResult : public LexicalAnalyzer::ParsingResult
	{
	public:
		ConnectFailParsingResult() : ParsingResult( ParserResultType::ConnectFail ) { linkId = 0; }

		int linkId;
	};

	class CMEErrorParsingResult : public LexicalAnalyzer::ParsingResult
	{
	public:
		CMEErrorParsingResult() : ParsingResult( ParserResultType::CMEError ) { code = 0; }

		int code;
	};

	class CPinParser : public LexicalAnalyzer::Parser
	{
	public:
		CPinParser() { state = State::WaitingHeader; }
	
	private:
		bool parse( const ByteRingIterator& begin, const ByteRingIterator& end ) override;
		void reset() override;

	private:
		enum class State { WaitingHeader, WaitingEndSeq } state;
		ByteRingIterator i;
		char str[20];
		CPinParsingResult res;
	};

	class ReceiveParser : public LexicalAnalyzer::Parser
	{
	public:
		ReceiveParser() { state = State::Unknown; dataSize = 0; }

	private:
		bool parse( const ByteRingIterator& begin, const ByteRingIterator& end ) override;
		void reset() override;
		int parseInt();

	private:
		enum class State { Unknown, WaitingHeader, WaitingData } state;
		ByteRingIterator i;
		uint32_t dataSize;
		ReceiveParsingResult res;
	};

	void parseIp( const ByteRingIterator& begin, const ByteRingIterator& end, uint8_t addr[4] );

	class RemoteIpParser : public LexicalAnalyzer::Parser
	{
	public:
		RemoteIpParser() { state = State::WaitingHeader; }

	private:
		bool parse( const ByteRingIterator& begin, const ByteRingIterator& end ) override;
		void reset() override;

	private:
		enum class State { WaitingHeader, WaitingEndSeq } state;
		ByteRingIterator i;
		RemoteIpParsingResult res;
	};

	class IpParser : public LexicalAnalyzer::Parser
	{
	public:
		IpParser() { state = State::WaitingHeader; }

	private:
		bool parse( const ByteRingIterator& begin, const ByteRingIterator& end ) override;
		void reset() override;

	private:
		enum class State { WaitingHeader, WaitingEndSeq } state;
		ByteRingIterator i;
		IpParsingResult res;
	};

	class ClosedParser : public LexicalAnalyzer::Parser
	{
	private:
		bool parse( const ByteRingIterator& begin, const ByteRingIterator& end ) override;

	private:
		ClosedParsingResult res;
	};

	class CloseOkParser : public LexicalAnalyzer::Parser
	{
	private:
		bool parse( const ByteRingIterator& begin, const ByteRingIterator& end ) override;

	private:
		CloseOkParsingResult res;
	};

	class AlreadyConnectParser : public LexicalAnalyzer::Parser
	{
	private:
		bool parse( const ByteRingIterator& begin, const ByteRingIterator& end ) override;

	private:
		AlreadyConnectParsingResult res;
	};

	class ConnectOkParser : public LexicalAnalyzer::Parser
	{
	private:
		bool parse( const ByteRingIterator& begin, const ByteRingIterator& end ) override;

	private:
		ConnectOkParsingResult res;
	};

	class ConnectFailParser : public LexicalAnalyzer::Parser
	{
	private:
		bool parse( const ByteRingIterator& begin, const ByteRingIterator& end ) override;

	private:
		ConnectFailParsingResult res;
	};

	class CMEErrorParser : public LexicalAnalyzer::Parser
	{
	public:
		CMEErrorParser() { state = State::WaitingHeader; }

	private:
		bool parse( const ByteRingIterator& begin, const ByteRingIterator& end ) override;
		void reset() override;

	private:
		enum class State { WaitingHeader, WaitingEndSeq } state;
		ByteRingIterator i;
		CMEErrorParsingResult res;
	};
}