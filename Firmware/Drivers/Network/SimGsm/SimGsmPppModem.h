#pragma once

#include "Network/AbstractPppModem.h"
#include "Drivers/LogicOutput.h"

#include "SimGsmATResponseParsers.h"

class SimGsmPppModem : public AbstractPppModem
{
public:
	SimGsmPppModem( IOPort enablePin, bool usePwrKeyPin = true, uint32_t enableLevel = 1 );
	~SimGsmPppModem();

	bool startModem( tprio_t prio = NORMALPRIO ) override;

private:
	LowLevelError lowLevelStart() override;
	void lowLevelStop() override;

	int executeSetting( uint32_t lexicalAnalyzerState, const char* cmd, sysinterval_t timeout = TIME_S2I( 5 ) );
	int waitForCPinOrPwrDown( sysinterval_t timeout = TIME_S2I( 8 ) );
	int waitForReadyFlags(); // Call and Sms

	void lexicalAnalyzerCallback( LexicalAnalyzer::ParsingResult* res );

	inline int printCPIN( char* str );
	inline int printCGDCONT( char* str );
	static void timerCallback( void* p );

private:
	enum : eventmask_t { TimeoutEvent = 2, UsartEvent = 4 };
	enum AnalyzerToken { CpinToken, CallReadyToken, SmsReadyToken, OkToken, ConnectToken, ErrorToken, CmeErrorToken, PwrDownToken };
	enum AnalyzerState { InitAnalyzerState = 1, SettingAnalyzerState };
	enum ErrorCode { NoError, TimeoutError, OverflowError, GeneralError, StopError, CMEError };

	LogicOutput enablePin;
	volatile bool usePwrKeyPin, pwrDown, callReady, smsReady;

	SimGsmATResponseParsers::CPinParsingResult::Status cpinStatus;
	enum class Status { Executing, Ok, Error } status;
	int errCode;

	LexicalAnalyzer lexicalAnalyzer;
	SimGsmATResponseParsers::CPinParser cpinParser;
	SimGsmATResponseParsers::CMEErrorParser cmeErrorParser;
};