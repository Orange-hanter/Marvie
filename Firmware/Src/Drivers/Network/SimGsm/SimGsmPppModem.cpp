#include "SimGsmPppModem.h"
#include "Core/Assert.h"
#include "Support/Utility.h"
#include <string.h>

using namespace Utility;
using namespace SimGsmATResponseParsers;

SimGsmPppModem::SimGsmPppModem( IOPort enableIOPort, bool usePwrKeyPin /*= true*/, uint32_t enableLevel /*= 1 */ )
	: AbstractPppModem( 1280 ), lexicalAnalyzer( 8, 2 )
{
	this->usePwrKeyPin = usePwrKeyPin;
	if( usePwrKeyPin )
	{
		enablePin.attach( enableIOPort );
		enablePin.on();
	}
	else
		enablePin.attach( enableIOPort, !enableLevel );
	pwrDown = callReady = smsReady = false;
	cpinStatus = CPinParsingResult::Status::Unknown;
	status = Status::Executing;
	errCode = -1;

	lexicalAnalyzer.token( CpinToken )->setName( "+CPIN" );
	lexicalAnalyzer.token( CpinToken )->setParser( &cpinParser );
	lexicalAnalyzer.token( CallReadyToken )->setName( "Call Ready\r\n" );
	lexicalAnalyzer.token( CallReadyToken )->setParsingResultType( ParserResultType::CallReady );
	lexicalAnalyzer.token( SmsReadyToken )->setName( "SMS Ready\r\n" );
	lexicalAnalyzer.token( SmsReadyToken )->setParsingResultType( ParserResultType::SmsReady );
	lexicalAnalyzer.token( OkToken )->setName( "OK\r\n" );
	lexicalAnalyzer.token( OkToken )->setParsingResultType( ParserResultType::Ok );
	lexicalAnalyzer.token( ConnectToken )->setName( "CONNECT\r\n" );
	lexicalAnalyzer.token( ConnectToken )->setParsingResultType( ParserResultType::Connect );
	lexicalAnalyzer.token( ErrorToken )->setName( "ERROR\r\n" );
	lexicalAnalyzer.token( ErrorToken )->setParsingResultType( ParserResultType::Error );
	lexicalAnalyzer.token( CmeErrorToken )->setName( "+CME ERROR" );
	lexicalAnalyzer.token( CmeErrorToken )->setParser( &cmeErrorParser );
	lexicalAnalyzer.token( PwrDownToken )->setName( "NORMAL POWER DOWN\r\n" );
	lexicalAnalyzer.token( PwrDownToken )->setParsingResultType( ParserResultType::PwrDown );

	lexicalAnalyzer.state( InitAnalyzerState )->addRelatedToken( lexicalAnalyzer.token( CpinToken ) ).
		addRelatedToken( lexicalAnalyzer.token( CallReadyToken ) ).
		addRelatedToken( lexicalAnalyzer.token( SmsReadyToken ) ).
		addRelatedToken( lexicalAnalyzer.token( PwrDownToken ) );
	lexicalAnalyzer.state( SettingAnalyzerState )->addRelatedToken( lexicalAnalyzer.token( OkToken ) ).
		addRelatedToken( lexicalAnalyzer.token( ConnectToken ) ).
		addRelatedToken( lexicalAnalyzer.token( ErrorToken ) ).
		addRelatedToken( lexicalAnalyzer.token( CmeErrorToken ) );

	lexicalAnalyzer.setUniversumStateEnabled( false );
	lexicalAnalyzer.setCallback( [this]( LexicalAnalyzer::ParsingResult* res ) { lexicalAnalyzerCallback( res ); } );
}

SimGsmPppModem::~SimGsmPppModem()
{

}

bool SimGsmPppModem::startModem( tprio_t prio /*= NORMALPRIO */ )
{
	if( mApn == nullptr )
		return false;
	return AbstractPppModem::startModem( prio );
}

AbstractPppModem::LowLevelError SimGsmPppModem::lowLevelStart()
{
#define SHUTDOWN_AND_RETURN( err ) { lowLevelStop(); return err; }
	int err;
	lexicalAnalyzer.setBuffer( usart->inputBuffer() );
	/*if( usePwrKeyPin && !enablePin.state() )
	{
		enablePin.on();
		chThdSleepMilliseconds( 800 );
	}*/

GsmReinit:
	enablePin.off();
	if( usePwrKeyPin )
		chThdSleepMilliseconds( 1500 );
	else
		chThdSleepMilliseconds( 100 );
	usart->reset();
	lexicalAnalyzer.reset();
	enablePin.on();

	err = waitForCPinOrPwrDown();
	if( err == ErrorCode::StopError )
		SHUTDOWN_AND_RETURN( LowLevelError::StopRequest );
	if( pwrDown || err == ErrorCode::TimeoutError || err == ErrorCode::OverflowError )
	{
		if( pwrDown )
			chThdSleepMilliseconds( 800 );
		goto GsmReinit;
	}

	char str[70];
	/*while( cpinStatus == CPinParsingResult::Status::NotInserted )
	{
		err = waitForCPinOrPwrDown( TIME_INFINITE );
		if( err == ErrorCode::StopError )
			SHUTDOWN_AND_RETURN( LowLevelError::StopRequest );
		if( err == ErrorCode::OverflowError )
			goto GsmReinit;
	}*/
	if( cpinStatus == CPinParsingResult::Status::SimPin )
	{
		str[printCPIN( str )] = 0;
		int err = executeSetting( SettingAnalyzerState, str );
		if( err == StopError )
			SHUTDOWN_AND_RETURN( LowLevelError::StopRequest );
		if( err == GeneralError )
			SHUTDOWN_AND_RETURN( LowLevelError::PinCodeError );
		if( err != ErrorCode::NoError )
			goto GsmReinit;
	}
	else if( cpinStatus == CPinParsingResult::Status::NotInserted )
		goto GsmReinit /*piece of shit*/; // SHUTDOWN_AND_RETURN( LowLevelError::SimCardNotInsertedError )
	else if( cpinStatus != CPinParsingResult::Status::Ready )
		//SHUTDOWN_AND_RETURN( LowLevelError::PinCodeError );
		goto GsmReinit;

	err = waitForReadyFlags();
	if( err == ErrorCode::StopError )
		SHUTDOWN_AND_RETURN( LowLevelError::StopRequest );
	if( ( err == ErrorCode::TimeoutError && !( smsReady || callReady ) ) || err == ErrorCode::OverflowError )
		goto GsmReinit;

	str[printCGDCONT( str )] = 0;
	err = executeSetting( SettingAnalyzerState, str );
	if( err == ErrorCode::StopError )
		SHUTDOWN_AND_RETURN( LowLevelError::StopRequest );
	if( err != ErrorCode::NoError )
		goto GsmReinit;
	err = executeSetting( SettingAnalyzerState, "ATD*99#\r\n" );
	if( err == ErrorCode::StopError )
		SHUTDOWN_AND_RETURN( LowLevelError::StopRequest );
	if( err != ErrorCode::NoError )
		goto GsmReinit;

	chThdSleepMilliseconds( 1000 );
	return LowLevelError::NoError;
}

void SimGsmPppModem::lowLevelStop()
{
	if( usePwrKeyPin )
	{
		while( !pwrDown )
		{
			if( !enablePin.state() )
			{
				enablePin.on();
				chThdSleepMilliseconds( 800 );
			}
			enablePin.off();
			chThdSleepMilliseconds( 1500 );
			usart->inputBuffer()->clear();
			usart->inputBuffer()->resetOverflowFlag();
			enablePin.on();
			waitForCPinOrPwrDown();
		}
	}
	else
	{
		if( enablePin.state() )
		{
			enablePin.off();
			chThdSleepMilliseconds( 100 );
		}
	}
}

int SimGsmPppModem::executeSetting( uint32_t lexicalAnalyzerState, const char* cmd, sysinterval_t timeout /*= TIME_S2I( 5 ) */ )
{
#define BREAK( v ) { res = v; break; }
	int res;
	errCode = -1;
	status = Status::Executing;
	lexicalAnalyzer.setState( lexicalAnalyzerState );
	chEvtGetAndClearEvents( ~StopRequestEvent );

	usart->write( ( const uint8_t* )cmd, strlen( cmd ), TIME_INFINITE );

	virtual_timer_t timer;
	chVTObjectInit( &timer );
	chVTSet( &timer, timeout, timerCallback, this );

	EventListener usartListener;
	usart->eventSource().registerMaskWithFlags( &usartListener, UsartEvent, CHN_INPUT_AVAILABLE );
	while( true )
	{
		lexicalAnalyzer.processNewBytes();
		if( usart->inputBuffer()->isOverflowed() )
			BREAK( ErrorCode::OverflowError );

		if( status == Status::Executing )
		{
			eventmask_t em = chEvtWaitAny( UsartEvent );
			if( em & TimeoutEvent )
				BREAK( ErrorCode::TimeoutError );		
			if( em & StopRequestEvent )
				BREAK( ErrorCode::StopError );
		}
		else if( status == Status::Ok )
			BREAK( ErrorCode::NoError )
		else // req.status == Status::Error
			BREAK( errCode );
	}

	chVTReset( &timer );

	return res;
}

int SimGsmPppModem::waitForCPinOrPwrDown( sysinterval_t timeout )
{
#define BREAK( v ) { res = v; break; }
	int res;
	chEvtGetAndClearEvents( ~StopRequestEvent );

	pwrDown = false;
	cpinStatus = CPinParsingResult::Status::Unknown;
	lexicalAnalyzer.setState( InitAnalyzerState );

	virtual_timer_t timer;
	chVTObjectInit( &timer );
	chVTSet( &timer, timeout, timerCallback, this );

	EventListener usartListener;
	usart->eventSource().registerMaskWithFlags( &usartListener, UsartEvent, CHN_INPUT_AVAILABLE );

	while( true )
	{
		lexicalAnalyzer.processNewBytes();
		if( usart->inputBuffer()->isOverflowed() )
			BREAK( ErrorCode::OverflowError );
		if( pwrDown || cpinStatus != CPinParsingResult::Status::Unknown )
			BREAK( ErrorCode::NoError );

		eventmask_t em = chEvtWaitAny( StopRequestEvent | TimeoutEvent | UsartEvent );
		if( em & StopRequestEvent )
			BREAK( ErrorCode::StopError );
		if( em & TimeoutEvent )
			BREAK( ErrorCode::TimeoutError );
	}

	chVTReset( &timer );

	return res;
}

int SimGsmPppModem::waitForReadyFlags()
{
#define BREAK( v ) { res = v; break; }
	int res;
	chEvtGetAndClearEvents( ~StopRequestEvent );

	callReady = smsReady = false;
	lexicalAnalyzer.setState( InitAnalyzerState );

	virtual_timer_t timer;
	chVTObjectInit( &timer );
	chVTSet( &timer, TIME_S2I( 8 ), timerCallback, this );

	EventListener usartListener;
	usart->eventSource().registerMaskWithFlags( &usartListener, UsartEvent, CHN_INPUT_AVAILABLE );

	while( true )
	{
		lexicalAnalyzer.processNewBytes();
		if( usart->inputBuffer()->isOverflowed() )
			BREAK( ErrorCode::OverflowError );
		if( callReady && smsReady )
			BREAK( ErrorCode::NoError );

		eventmask_t em = chEvtWaitAny( StopRequestEvent | TimeoutEvent | UsartEvent );
		if( em & StopRequestEvent )
			BREAK( ErrorCode::StopError );
		if( em & TimeoutEvent )
			BREAK( ErrorCode::TimeoutError );
	}

	chVTReset( &timer );

	return res;
}

void SimGsmPppModem::lexicalAnalyzerCallback( LexicalAnalyzer::ParsingResult* res )
{
	ParserResultType type = ( ParserResultType )res->type();
	switch( type )
	{
	case SimGsmATResponseParsers::Ok:
	{
		status = Status::Ok;
		break;
	}
	case SimGsmATResponseParsers::Connect:
	{
		status = Status::Ok;
		break;
	}
	case SimGsmATResponseParsers::CPin:
	{
		cpinStatus = static_cast< CPinParsingResult* >( res )->status;
		break;
	}
	case SimGsmATResponseParsers::PwrDown:
	{
		pwrDown = true;
		break;
	}
	case SimGsmATResponseParsers::CallReady:
	{
		callReady = true;
		break;
	}
	case SimGsmATResponseParsers::SmsReady:
	{
		smsReady = true;
		break;
	}
	case SimGsmATResponseParsers::Error:
	{
		status = Status::Error;
		errCode = ErrorCode::GeneralError;
		break;
	}
	case SimGsmATResponseParsers::CMEError:
	{
		status = Status::Error;
		errCode = static_cast< CMEErrorParsingResult* >( res )->code + ErrorCode::CMEError;
		break;
	}
	default:
		assert( false );
		break;
	}
}

int SimGsmPppModem::printCPIN( char* str )
{
	uint32_t pinCode = this->mPinCode;
	memcpy( str, "AT+CPIN=", 8 );
	char* cmd = Utility::printInt( str + 8, pinCode );
	*cmd++ = '\r', *cmd++ = '\n';

	return cmd - str;
}

int SimGsmPppModem::printCGDCONT( char* str )
{
	const char* apn = this->mApn;
	memcpy( str, "AT+CGDCONT=1,\"IP\",\"", 19 );
	size_t len = strlen( apn );
	memcpy( str + 19, apn, len );
	memcpy( str + 19 + len, "\"\r\n", 3 );

	return len + 22;
}

void SimGsmPppModem::timerCallback( void* p )
{
	chSysLockFromISR();
	reinterpret_cast< SimGsmPppModem* >( p )->signalEventsI( TimeoutEvent );
	chSysUnlockFromISR();
}