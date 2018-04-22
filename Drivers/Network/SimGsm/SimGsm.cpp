#include "SimGsm.h"
#include "Support/Utility.h"
#include "Core/Assert.h"
#include <string.h>

#include "SimGsmUdpSocket.h"
#include "SimGsmTcpSocket.h"

#define DEFAULT_INPUT_BUFFER_SIZE   32
#define DEFAULT_OUTPUT_BUFFER_SIZE  0
#define MAX_UDP_PACKET_SIZE         2048
#define MAX_TCP_PACKET_SIZE         2048
#define PING_DELAY                  TIME_S2I( 12 )

#define INIT_ANALYZER_STATE      1
#define AT_ANALYZER_STATE        2
#define CIPMUX_ANALYZER_STATE    2
#define CPIN_ANALYZER_STATE      3
#define CSTT_ANALYZER_STATE      3
#define CIICR_ANALYZER_STATE     3
#define CIFSR_ANALYZER_STATE     4
#define CIPSRIP_ANALYZER_STATE   5
#define UDPMODE_ANALYZER_STATE   5
#define CLPORT_ANALYZER_STATE    5
#define CIPSERVER_ANALYZER_STATE 6
#define CIPSEND_ANALYZER_STATE   7
#define DATA_ANALYZER_STATE      8
#define CIPCLOSE_ANALYZER_STATE  9
#define CIPSTART_ANALYZER_STATE  10

using namespace Utility;
using namespace SimGsmATResponseParsers;

SimGsm::SimGsm( IOPort port ) : lexicalAnalyzer( 21, 10 )
{
	mStatus = ModemStatus::Stopped;
	mError = ModemError::NoError;
	chThdQueueObjectInit( &waitingQueue );
	chVTObjectInit( &responseTimer );
	chVTObjectInit( &modemPingTimer );

	usart = nullptr;
#ifdef SIMGSM_USE_PWRKEY_PIN
	enablePin.attach( port );
	enablePin.on();
#else
	enablePin.attach( port, !uint32_t( SIMGSM_ENABLE_LEVEL ) );
#endif
	pinCode = 0;
	apn = nullptr;
	pwrDown = crashFlag = callReady = smsReady = false;
	cpinStatus = CPinParsingResult::Status::Unknown;
	sendReqErrorLinkId = -1;
	sendReqState = SendRequestState::SetRemoteAddress;
	sendReqDataTime = 0;

	for( int i = 0; i < SIMGSM_SOCKET_LIMIT; ++i )
	{
		linkDesc[i].reserved = false;
		linkDesc[i].socket = nullptr;
	}
	server = nullptr;

	currentRequest = nullptr;
	atReqNode.value = &atReq;
	dataSend[0] = dataSend[1] = nullptr;
	dataSendSize[0] = dataSendSize[1] = 0;

	lexicalAnalyzer.token( ParserResultType::CPin )->setName( "+CPIN" );
	lexicalAnalyzer.token( ParserResultType::CPin )->setParser( &cpinParser );
	lexicalAnalyzer.token( ParserResultType::CallReady )->setName( "Call Ready\r\n" );
	lexicalAnalyzer.token( ParserResultType::CallReady )->setParsingResultType( ParserResultType::CallReady );
	lexicalAnalyzer.token( ParserResultType::SmsReady )->setName( "SMS Ready\r\n" );
	lexicalAnalyzer.token( ParserResultType::SmsReady )->setParsingResultType( ParserResultType::SmsReady );
	lexicalAnalyzer.token( ParserResultType::PdpDeact )->setName( "+PDP: DEACT\r\n" );
	lexicalAnalyzer.token( ParserResultType::PdpDeact )->setParsingResultType( ParserResultType::PdpDeact );
	lexicalAnalyzer.token( ParserResultType::Receive )->setName( "+RECEIVE" );
	lexicalAnalyzer.token( ParserResultType::Receive )->setParser( &receiveParser );
	lexicalAnalyzer.token( ParserResultType::RemoteIp )->setName( "REMOTE IP" );
	lexicalAnalyzer.token( ParserResultType::RemoteIp )->setParser( &remoteIpParser );
	lexicalAnalyzer.token( ParserResultType::Ip )->setName( "\r\n" );
	lexicalAnalyzer.token( ParserResultType::Ip )->setParser( &ipParser );
	lexicalAnalyzer.token( ParserResultType::Closed )->setName( "CLOSED\r\n" );
	lexicalAnalyzer.token( ParserResultType::Closed )->setParser( &closedParser );
	lexicalAnalyzer.token( ParserResultType::Ok )->setName( "OK\r\n" );
	lexicalAnalyzer.token( ParserResultType::Ok )->setParsingResultType( ParserResultType::Ok );
	lexicalAnalyzer.token( ParserResultType::ServerOk )->setName( "SERVER OK\r\n" );
	lexicalAnalyzer.token( ParserResultType::ServerOk )->setParsingResultType( ParserResultType::ServerOk );
	lexicalAnalyzer.token( ParserResultType::ServerClose )->setName( "SERVER CLOSE\r\n" );
	lexicalAnalyzer.token( ParserResultType::ServerClose )->setParsingResultType( ParserResultType::ServerClose );
	lexicalAnalyzer.token( ParserResultType::SendInitOk )->setName( "\r\n> " );
	lexicalAnalyzer.token( ParserResultType::SendInitOk )->setParsingResultType( ParserResultType::SendInitOk );
	lexicalAnalyzer.token( ParserResultType::SendOk )->setName( "SEND OK\r\n" );
	lexicalAnalyzer.token( ParserResultType::SendOk )->setParsingResultType( ParserResultType::SendOk );
	lexicalAnalyzer.token( ParserResultType::SendFail )->setName( "SEND FAIL\r\n" );
	lexicalAnalyzer.token( ParserResultType::SendFail )->setParsingResultType( ParserResultType::SendFail );
	lexicalAnalyzer.token( ParserResultType::CloseOk )->setName( "CLOSE OK\r\n" );
	lexicalAnalyzer.token( ParserResultType::CloseOk )->setParser( &closeOkParser );
	lexicalAnalyzer.token( ParserResultType::AlreadyConnect )->setName( "ALREADY CONNECT\r\n" );
	lexicalAnalyzer.token( ParserResultType::AlreadyConnect )->setParser( &alreadyConnectParser );
	lexicalAnalyzer.token( ParserResultType::ConnectOk )->setName( "CONNECT OK\r\n" );
	lexicalAnalyzer.token( ParserResultType::ConnectOk )->setParser( &connectOkParser );
	lexicalAnalyzer.token( ParserResultType::ConnectFail )->setName( "CONNECT FAIL\r\n" );
	lexicalAnalyzer.token( ParserResultType::ConnectFail )->setParser( &connectFailParser );
	lexicalAnalyzer.token( ParserResultType::Error )->setName( "ERROR\r\n" );
	lexicalAnalyzer.token( ParserResultType::Error )->setParsingResultType( ParserResultType::Error );
	lexicalAnalyzer.token( ParserResultType::CMEError )->setName( "+CME ERROR" );
	lexicalAnalyzer.token( ParserResultType::CMEError )->setParser( &cmeErrorParser );
	lexicalAnalyzer.token( ParserResultType::PwrDown )->setName( "NORMAL POWER DOWN\r\n" );
	lexicalAnalyzer.token( ParserResultType::PwrDown )->setParsingResultType( ParserResultType::PwrDown );

	lexicalAnalyzer.state( 0 )->addRelatedToken( lexicalAnalyzer.token( ParserResultType::Receive ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::RemoteIp ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::Closed ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::PdpDeact ) );
	lexicalAnalyzer.state( 1 )->addRelatedToken( lexicalAnalyzer.token( ParserResultType::CPin ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::CallReady ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::SmsReady ) );
#ifdef SIMGSM_USE_PWRKEY_PIN
	lexicalAnalyzer.state( 1 )->addRelatedToken( lexicalAnalyzer.token( ParserResultType::PwrDown ) );
#endif
	lexicalAnalyzer.state( 2 )->addRelatedToken( lexicalAnalyzer.token( ParserResultType::Ok ) );
	lexicalAnalyzer.state( 3 )->addRelatedToken( lexicalAnalyzer.token( ParserResultType::Ok ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::Error ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::CMEError ) );
	lexicalAnalyzer.state( 4 )->addRelatedToken( lexicalAnalyzer.token( ParserResultType::Ip ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::Error ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::CMEError ) );
	lexicalAnalyzer.state( 5 )->addRelatedToken( lexicalAnalyzer.token( ParserResultType::Ok ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::Error ) );
	lexicalAnalyzer.state( 6 )->addRelatedToken( lexicalAnalyzer.token( ParserResultType::ServerOk ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::ServerClose ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::Error ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::CMEError ) );
	lexicalAnalyzer.state( 7 )->addRelatedToken( lexicalAnalyzer.token( ParserResultType::SendInitOk ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::Error ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::CMEError ) );
	lexicalAnalyzer.state( 8 )->addRelatedToken( lexicalAnalyzer.token( ParserResultType::SendOk ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::SendFail ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::Error ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::CMEError ) );
	lexicalAnalyzer.state( 9 )->addRelatedToken( lexicalAnalyzer.token( ParserResultType::CloseOk ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::Error ) ); // ?
	lexicalAnalyzer.state( 10 )->addRelatedToken( lexicalAnalyzer.token( ParserResultType::AlreadyConnect ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::ConnectOk ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::ConnectFail ) ).
		addRelatedToken( lexicalAnalyzer.token( ParserResultType::CMEError ) );

	lexicalAnalyzer.setUniversumStateEnabled( false );
	lexicalAnalyzer.setCallback( [this]( LexicalAnalyzer::ParsingResult* res ) { lexicalAnalyzerCallback( res ); } );
}

SimGsm::~SimGsm()
{
	stopModem();
	waitForStatusChange();
	delete server;
}

void SimGsm::setUsart( Usart* usart )
{
	if( mStatus != ModemStatus::Stopped )
		return;

	this->usart = usart;
	lexicalAnalyzer.setBuffer( usart->inputBuffer() );
}

void SimGsm::setPinCode( uint32_t pinCode )
{
	this->pinCode = pinCode;
}

void SimGsm::setApn( const char* apn )
{
	this->apn = apn;
}

void SimGsm::startModem( tprio_t prio )
{
	if( mStatus != ModemStatus::Stopped || !usart )
		return;

	mStatus = ModemStatus::Initializing;
	extEventSource.broadcastFlags( ( eventflags_t )ModemEvent::StatusChanged );
	start( prio );
}

void SimGsm::stopModem()
{
	chSysLock();
	if( mStatus == ModemStatus::Stopped || mStatus == ModemStatus::Stopping )
	{
		chSysUnlock();
		return;
	}
	mStatus = ModemStatus::Stopping;
	extEventSource.broadcastFlagsI( ( eventflags_t )ModemEvent::StatusChanged );
	innerEventSource.broadcastFlagsI( InnerEventFlag::StopRequestFlag );
	chSchRescheduleS();
	chSysUnlock();
}

bool SimGsm::waitForStatusChange( sysinterval_t timeout )
{
	msg_t msg = MSG_OK;
	chSysLock();
	if( mStatus == ModemStatus::Initializing || mStatus == ModemStatus::Stopping )
		msg = chThdEnqueueTimeoutS( &waitingQueue, timeout );
	chSysUnlock();

	return msg == MSG_OK;
}

ModemStatus SimGsm::status()
{
	return mStatus;
}

ModemError SimGsm::modemError()
{
	return mError;
}

IpAddress SimGsm::networkAddress()
{
	return netAddress;
}

AbstractTcpServer* SimGsm::tcpServer( uint32_t index )
{
	if( index != 0 )
		return nullptr;
	if( server )
		return server;
	server = new SimGsmTcpServer( this );
	return server;
}

AbstractUdpSocket* SimGsm::createUdpSocket()
{
	return new SimGsmUdpSocket( this, DEFAULT_INPUT_BUFFER_SIZE, DEFAULT_OUTPUT_BUFFER_SIZE );
}

AbstractUdpSocket* SimGsm::createUdpSocket( uint32_t inputBufferSize, uint32_t outputBufferSize )
{
	return new SimGsmUdpSocket( this, inputBufferSize, outputBufferSize );
}

AbstractTcpSocket* SimGsm::createTcpSocket()
{
	return new SimGsmTcpSocket( this, DEFAULT_INPUT_BUFFER_SIZE, DEFAULT_OUTPUT_BUFFER_SIZE );
}

AbstractTcpSocket* SimGsm::createTcpSocket( uint32_t inputBufferSize, uint32_t outputBufferSize )
{
	return new SimGsmTcpSocket( this, inputBufferSize, outputBufferSize );
}

EvtSource* SimGsm::eventSource()
{
	return &extEventSource;
}

void SimGsm::main()
{
Start:
	while( mStatus == ModemStatus::Initializing )
	{
		auto res = reinit();
		if( res == InitResult::Ok )
		{
			chSysLock();
			if( mStatus == ModemStatus::Stopping )
			{
				mStatus = ModemStatus::Stopped;
				extEventSource.broadcastFlagsI( ( eventflags_t )ModemEvent::StatusChanged );
				chThdDequeueNextI( &waitingQueue, MSG_OK );
				exitS( MSG_OK );
			}
			mStatus = ModemStatus::Working;
			extEventSource.broadcastFlagsI( ( eventflags_t )ModemEvent::StatusChanged );
			chThdDequeueNextI( &waitingQueue, MSG_OK );
			chSchRescheduleS();
			chSysUnlock();
		}
		else if( res == InitResult::PinCodeIncorrect )
		{
			chSysLock();
			mError = ModemError::AuthenticationError;
			mStatus = ModemStatus::Stopped;
			extEventSource.broadcastFlagsI( ( eventflags_t )ModemEvent::Error );
			extEventSource.broadcastFlagsI( ( eventflags_t )ModemEvent::StatusChanged );
			chThdDequeueNextI( &waitingQueue, MSG_OK );
			exitS( MSG_OK );
		}
	}

	chVTSet( &modemPingTimer, PING_DELAY, modemPingCallback, this );
	EvtListener usartListener, innerListener;
	enum Event { UsartEvent = EVENT_MASK( 0 ), InnerEvent = EVENT_MASK( 1 ) };
	usart->eventSource()->registerMaskWithFlags( &usartListener, UsartEvent, CHN_INPUT_AVAILABLE | CHN_OUTPUT_EMPTY );
	innerEventSource.registerMask( &innerListener, InnerEvent );
	lexicalAnalyzer.setUniversumStateEnabled( true );
	crashFlag = false;
	sendReqErrorLinkId = -1;
	dataSend[0] = dataSend[1] = nullptr;
	dataSendSize[0] = dataSendSize[1] = 0;

	while( mStatus == ModemStatus::Working )
	{
		lexicalAnalyzer.processNewBytes();
		if( crashFlag || usart->isInputBufferOverflowed() )
		{
Reinit:
			chSysLock();
			if( mStatus == ModemStatus::Stopping )
			{
				chSysUnlock();
				break;
			}
			closeAllS();
			mStatus = ModemStatus::Initializing;
			extEventSource.broadcastFlagsI( ( eventflags_t )ModemEvent::StatusChanged );
			chSchRescheduleS();
			chSysUnlock();
			innerEventSource.unregister( &innerListener );
			usart->eventSource()->unregister( &usartListener );
			goto Start;
		}

		eventmask_t em = chEvtWaitAny( UsartEvent | InnerEvent );
		if( em & UsartEvent )
		{
			eventflags_t flags = usartListener.getAndClearFlags();
			if( flags & CHN_OUTPUT_EMPTY )
				nextSend();
		}
		if( em & InnerEvent )
		{
			eventflags_t flags = innerListener.getAndClearFlags();
			if( flags & InnerEventFlag::TimeoutEventFlag )
				goto Reinit;
			if( flags & InnerEventFlag::NewRequestEventFlag )
			{
				assert( currentRequest == nullptr );
				nextRequest();
			}
			if( flags & InnerEventFlag::ModemPingEventFlag && !atReq.use )
			{
				if( addRequest( &atReqNode ) )
					atReq.use = true;
			}
		}
	}

	innerEventSource.unregister( &innerListener );
	usart->eventSource()->unregister( &usartListener );

	chSysLock();
	closeAllS();
	chSysUnlock();
	shutdown();
	chSysLock();
	mStatus = ModemStatus::Stopped;
	extEventSource.broadcastFlagsI( ( eventflags_t )ModemEvent::StatusChanged );
	chThdDequeueNextI( &waitingQueue, MSG_OK );
	exitS( MSG_OK );
}

SimGsm::InitResult SimGsm::reinit()
{
	if( apn == nullptr || usart == nullptr )
		return InitResult::Fail;

#ifdef SIMGSM_USE_PWRKEY_PIN
	if( !enablePin.state() )
	{
		enablePin.on();
		chThdSleepMilliseconds( 800 );
	}
#endif
GsmReinit:
	enablePin.off();
#ifdef SIMGSM_USE_PWRKEY_PIN
	chThdSleepMilliseconds( 1500 );
#else
	chThdSleepMilliseconds( 10 );
#endif
	usart->reset();
	lexicalAnalyzer.reset();
	lexicalAnalyzer.setUniversumStateEnabled( false );
	enablePin.on();

	waitForCPinOrPwrDown();
#ifdef SIMGSM_USE_PWRKEY_PIN
	if( pwrDown )
	{
		chThdSleepMilliseconds( 800 );
		goto GsmReinit;
	}
#endif
	if( cpinStatus == CPinParsingResult::Status::SimPin )
	{
		str[printCPIN()] = 0;
		if( executeSetting( CPIN_ANALYZER_STATE, str ) != 0 )
			return InitResult::PinCodeIncorrect;
	}
	else if( cpinStatus == CPinParsingResult::Status::Unknown )
		return InitResult::Fail;
	else if( cpinStatus != CPinParsingResult::Status::Ready )
		return InitResult::PinCodeIncorrect;

	waitForReadyFlags();

	if( mStatus != ModemStatus::Initializing || executeSetting( CIPMUX_ANALYZER_STATE, "AT+CIPMUX=1\r\n" ) != 0 )
		return InitResult::Fail;

	str[printCSTT()] = 0;
	if( mStatus != ModemStatus::Initializing || executeSetting( CSTT_ANALYZER_STATE, str, 1500, 3 ) != 0 )
		return InitResult::Fail;

	chThdSleepMilliseconds( 2000 );

	if( mStatus != ModemStatus::Initializing || executeSetting( CIICR_ANALYZER_STATE, "AT+CIICR\r\n", 0, 1, TIME_S2I( 10 ) ) != 0 )
		return InitResult::Fail;

	if( mStatus != ModemStatus::Initializing || executeSetting( CIFSR_ANALYZER_STATE, "AT+CIFSR\r\n" ) != 0 )
		return InitResult::Fail;

	if( mStatus != ModemStatus::Initializing || executeSetting( CIPSRIP_ANALYZER_STATE, "AT+CIPSRIP=1\r\n" ) != 0 )
		return InitResult::Fail;

	char udpMode[] = "AT+CIPUDPMODE=0,1\r\n";
	for( int i = 0; i < SIMGSM_SOCKET_LIMIT; ++i )
	{
		udpMode[14] = '0' + i;
		if( executeSetting( UDPMODE_ANALYZER_STATE, udpMode ) != 0 )
			return InitResult::Fail;
	}

	lexicalAnalyzer.setState( 0 );
	return InitResult::Ok;
}

int SimGsm::executeSetting( uint32_t lexicalAnalyzerState, const char* cmd, uint32_t delayMs /*= 100*/, uint32_t repeateCount /*= 8*/, sysinterval_t timeout /*= TIME_S2I( 5 )*/ )
{
	int res;
	GeneralRequest req;
	RequestNode reqNode( &req );
	currentRequest = &reqNode;
	lexicalAnalyzer.setState( lexicalAnalyzerState );

	usart->write( ( const uint8_t* )cmd, strlen( cmd ), TIME_INFINITE );
	// TODO: wait ?

	virtual_timer_t timer;
	chVTObjectInit( &timer );
	chVTSet( &timer, timeout, timeoutCallback, this );

	EvtListener usartListener, innerEventListener;
	enum Event : eventmask_t { UsartEvent = EVENT_MASK( 0 ), InnerEvent = EVENT_MASK( 1 ) };
	usart->eventSource()->registerMaskWithFlags( &usartListener, UsartEvent, CHN_INPUT_AVAILABLE );
	innerEventSource.registerMaskWithFlags( &innerEventListener, InnerEvent, TimeoutEventFlag );
	while( true )
	{
		lexicalAnalyzer.processNewBytes();
		if( usart->inputBuffer()->isOverflowed() )
		{
			res = ErrorCode::OverflowError;
			break;
		}

		if( req.status == GeneralRequest::Status::Executing )
		{
			eventmask_t em = chEvtWaitAny( UsartEvent | InnerEvent );
			if( em & InnerEvent )
			{
				res = ErrorCode::TimeoutError;
				break;
			}
			if( em & UsartEvent )
				usartListener.getAndClearFlags();
		}
		else if( req.status == GeneralRequest::Status::Ok )
		{
			res = 0;
			break;
		}
		else // req.status == GeneralRequest::Status::Error
		{
			if( --repeateCount == 0 )
			{
				res = req.errCode;
				break;
			}
			chThdSleepMilliseconds( delayMs );

			req.status = GeneralRequest::Status::Executing;
			req.errCode = -1;
			usart->write( ( const uint8_t* )cmd, strlen( cmd ), TIME_INFINITE );

			chVTSet( &timer, timeout, timeoutCallback, this );
			innerEventListener.getAndClearFlags();
		}
	}

	usart->eventSource()->unregister( &usartListener );
	innerEventSource.unregister( &innerEventListener );
	chVTReset( &timer );
	currentRequest = nullptr;
	return res;
}

void SimGsm::waitForCPinOrPwrDown()
{
	pwrDown = false;
	cpinStatus = CPinParsingResult::Status::Unknown;
	lexicalAnalyzer.setState( INIT_ANALYZER_STATE );

	virtual_timer_t timer;
	chVTObjectInit( &timer );
	chVTSet( &timer, TIME_S2I( 3 ), timeoutCallback, this );

	EvtListener usartListener, innerEventListener;
	enum Event : eventmask_t { UsartEvent = EVENT_MASK( 0 ), InnerEvent = EVENT_MASK( 1 ) };
	usart->eventSource()->registerMaskWithFlags( &usartListener, UsartEvent, CHN_INPUT_AVAILABLE );
	innerEventSource.registerMaskWithFlags( &innerEventListener, InnerEvent, TimeoutEventFlag );
	while( true )
	{
		lexicalAnalyzer.processNewBytes();
		if( pwrDown || cpinStatus != CPinParsingResult::Status::Unknown || usart->inputBuffer()->isOverflowed() )
			break;

		eventmask_t em = chEvtWaitAny( UsartEvent | InnerEvent );
		if( em & InnerEvent )
			break;
		if( em & UsartEvent )
			usartListener.getAndClearFlags();
	}

	usart->eventSource()->unregister( &usartListener );
	innerEventSource.unregister( &innerEventListener );
	chVTReset( &timer );
}

void SimGsm::waitForReadyFlags()
{
	callReady = smsReady = false;
	lexicalAnalyzer.setState( INIT_ANALYZER_STATE );

	virtual_timer_t timer;
	chVTObjectInit( &timer );
	chVTSet( &timer, TIME_S2I( 8 ), timeoutCallback, this );

	EvtListener usartListener, innerEventListener;
	enum Event : eventmask_t { UsartEvent = EVENT_MASK( 0 ), InnerEvent = EVENT_MASK( 1 ) };
	usart->eventSource()->registerMaskWithFlags( &usartListener, UsartEvent, CHN_INPUT_AVAILABLE );
	innerEventSource.registerMaskWithFlags( &innerEventListener, InnerEvent, TimeoutEventFlag );
	while( true )
	{
		lexicalAnalyzer.processNewBytes();
		if( ( callReady && smsReady ) || usart->inputBuffer()->isOverflowed() )
			break;

		eventmask_t em = chEvtWaitAny( UsartEvent | InnerEvent );
		if( em & InnerEvent )
			break;
		if( em & UsartEvent )
			usartListener.getAndClearFlags();
	}

	usart->eventSource()->unregister( &usartListener );
	innerEventSource.unregister( &innerEventListener );
	chVTReset( &timer );
}

void SimGsm::lexicalAnalyzerCallback( LexicalAnalyzer::ParsingResult* res )
{
	chVTSet( &modemPingTimer, PING_DELAY, modemPingCallback, this );
	ParserResultType type = ( ParserResultType )res->type();
	switch( type )
	{
	case SimGsmATResponseParsers::Receive:
	{
		SimGsmATResponseParsers::ReceiveParsingResult* rres = static_cast< SimGsmATResponseParsers::ReceiveParsingResult* >( res );
		AbstractSocket* socket = linkDesc[rres->linkId].socket;
		SimGsmSocketBase* base = static_cast< SimGsmUdpSocket* >( socket );
		if( socket->type() == SocketType::Udp )
		{
			if( base->inBuffer.writeAvailable() > sizeof( SimGsmUdpSocket::Header ) + rres->size )
			{
				SimGsmUdpSocket::Header header;
				header.datagramSize = rres->size;
				header.remoteAddress = IpAddress( rres->addr[3], rres->addr[2], rres->addr[1], rres->addr[0] );
				header.remotePort = rres->port;
				base->inBuffer.write( reinterpret_cast< uint8_t* >( &header ), sizeof( header ) );
				base->inBuffer.write( rres->begin, rres->size );
				base->eSource.broadcastFlags( ( eventflags_t )SocketEventFlag::InputAvailable );
			}
		}
		else
		{
			if( base->inBuffer.write( rres->begin, rres->size ) )
				base->eSource.broadcastFlags( ( eventflags_t )SocketEventFlag::InputAvailable );
		}
		break;
	}
	case SimGsmATResponseParsers::RemoteIp:
	{
		SimGsmATResponseParsers::RemoteIpParsingResult* rres = static_cast< SimGsmATResponseParsers::RemoteIpParsingResult* >( res );
		assert( rres->linkId < SIMGSM_SOCKET_LIMIT );
		assert( linkDesc[rres->linkId].socket == nullptr );
		SimGsmTcpSocket* socket = static_cast< SimGsmTcpSocket* >( createTcpSocket( server->inputBufferSize, server->outputBufferSize ) );
		chSysLock();
		linkDesc[rres->linkId].socket = socket;
		socket->connectHelpS( rres->linkId );
		server->addSocketS( socket );
		chSchRescheduleS();
		chSysUnlock();
		break;
	}
	case SimGsmATResponseParsers::Closed:
	{
		SimGsmATResponseParsers::ClosedParsingResult* cres = static_cast< SimGsmATResponseParsers::ClosedParsingResult* >( res );
		if( currentRequest &&
			( ( currentRequest->value->type == Request::Type::Close && static_cast< SimGsmUdpSocket* >( static_cast< CloseRequest* >( currentRequest->value )->socket )->linkId == cres->linkId ) ||
			( currentRequest->value->type == Request::Type::Send && static_cast< SimGsmUdpSocket* >( static_cast< SendRequest* >( currentRequest->value )->socket )->linkId == cres->linkId  &&
			  cres->linkId != sendReqErrorLinkId && sendReqState == SendRequestState::SendData && chVTIsSystemTimeWithinX( sendReqDataTime, sendReqDataTime + TIME_S2I( 2 ) ) ) ) )
			crash();
		else
		{
			volatile bool needNext = false;
			SimGsmUdpSocket* socket = static_cast< SimGsmUdpSocket* >( linkDesc[cres->linkId].socket );
			chSysLock();
			if( currentRequest && currentRequest->value->type == Request::Type::Send && static_cast< SendRequest* >( currentRequest->value )->socket == socket )
				requestList.pushFront( currentRequest ), needNext = true;
			// remove requests to closed socket
			for( auto i = requestList.begin(); i != requestList.end(); )
			{
				Request* req = *i;
				if( req->type == Request::Type::Close && static_cast< CloseRequest* >( req )->socket == socket )
				{
					chBSemResetI( &static_cast< CloseRequest* >( req )->semaphore, false );
					break;
				}
				if( req->type == Request::Type::Send && static_cast< SendRequest* >( req )->socket == socket )
				{
					if( !socket->outBuffer )
						chBSemResetI( &static_cast< SendRequest* >( req )->semaphore, false );
					auto irm = i++;
					requestList.remove( irm );
				}
				else
					++i;
			}
			socket->closeHelpS( SocketError::RemoteHostClosedError );
			linkDesc[cres->linkId].socket = nullptr;
			chSchRescheduleS();
			chSysUnlock();

			if( cres->linkId == sendReqErrorLinkId )
			{
				sendReqErrorLinkId = -1;
				chVTReset( &responseTimer );
				nextRequest();
			}
			else if( needNext )
				nextRequest();
		}
		break;
	}
	case SimGsmATResponseParsers::PdpDeact:
	{
		crash();
		break;
	}
	default:
		if( currentRequest )
		{
			chVTReset( &responseTimer );
			switch( currentRequest->value->type )
			{
			case Request::Type::Send:
				sendRequestHandler( res );
				break;
			case Request::Type::Start:
				startRequestHandler( res );
				break;
			case Request::Type::Close:
				closeRequestHandler( res );
				break;
			case Request::Type::At:
				atRequestHandler( res );
				break;
			case Request::Type::Server:
				serverRequestHandler( res );
				break;
			case Request::Type::General:
				generalRequestHandler( res );
				break;
			default:
				assert( false );
				break;
			}
		}
		else
		{
			// Init state tokens
			if( type == CPin )
				cpinStatus = static_cast< CPinParsingResult* >( res )->status;
			else if( type == PwrDown )
				pwrDown = true;
			else if( type == CallReady )
				callReady = true;
			else if( type == SmsReady )
				smsReady = true;
			else
				assert( false );
		}
	}
}

void SimGsm::generalRequestHandler( LexicalAnalyzer::ParsingResult* res )
{
	ParserResultType type = ( ParserResultType )res->type();
	GeneralRequest* req = static_cast< GeneralRequest* >( currentRequest->value );
	switch( type )
	{
	case SimGsmATResponseParsers::Ok:
	{
		req->status = GeneralRequest::Status::Ok;
		break;
	}
	case SimGsmATResponseParsers::Ip:
	{
		IpParsingResult* ipRes = static_cast< IpParsingResult* >( res );
		ip = IpAddress( ipRes->addr[3], ipRes->addr[2], ipRes->addr[1], ipRes->addr[0] );
		req->status = GeneralRequest::Status::Ok;
		break;
	}
	case SimGsmATResponseParsers::Error:
	{
		req->status = GeneralRequest::Status::Error;
		req->errCode = ErrorCode::GeneralError;
		break;
	}
	case SimGsmATResponseParsers::CMEError:
	{
		req->status = GeneralRequest::Status::Error;
		req->errCode = static_cast< CMEErrorParsingResult* >( res )->code + ErrorCode::CMEError;
		break;
	}
	default:
		assert( false );
		break;
	}
}

void SimGsm::sendRequestHandler( LexicalAnalyzer::ParsingResult* res )
{
	ParserResultType type = ( ParserResultType )res->type();
	SendRequest* req = static_cast< SendRequest* >( currentRequest->value );
	switch( type )
	{
	case SimGsmATResponseParsers::SendInitOk:
	{
		sendReqState = SendRequestState::SendData;
		sendReqDataTime = chVTGetSystemTimeX();
		lexicalAnalyzer.setState( DATA_ANALYZER_STATE );
		chVTSet( &responseTimer, TIME_S2I( 10 ), timeoutCallback, this );
		send( req->data[0], req->dataSize[0], req->data[1], req->dataSize[1] );
		break;
	}
	case SimGsmATResponseParsers::SendOk:
	{
		completePacketTransfer( false );
		nextRequest();
		break;
	}
	case SimGsmATResponseParsers::Ok:
	{
		SimGsmUdpSocket* udpSocket = static_cast< SimGsmUdpSocket* >( req->socket );
		udpSocket->cAddr = req->remoteAddress;
		udpSocket->cPort = req->remotePort;
		sendReqState = SendRequestState::SendInit;
		lexicalAnalyzer.setState( CIPSEND_ANALYZER_STATE );
		sendCommand( printCIPSEND( udpSocket->linkId, req->dataSize[0] + req->dataSize[1] ) );
		break;
	}
	case SimGsmATResponseParsers::SendFail:
	case SimGsmATResponseParsers::Error:
	{
		if( req->socket->type() == SocketType::Udp )
		{
			completePacketTransfer( true );
			nextRequest();
		}
		else
		{
			sendReqErrorLinkId = static_cast< SimGsmTcpSocket* >( req->socket )->linkId;
			chVTSet( &responseTimer, TIME_S2I( 10 ), timeoutCallback, this );
		}
		break;
	}
	case SimGsmATResponseParsers::CMEError:
	{
		crash();
		break;
	}
	default:
		assert( false );
		break;
	}
}

void SimGsm::startRequestHandler( LexicalAnalyzer::ParsingResult* res )
{
	ParserResultType type = ( ParserResultType )res->type();
	StartRequest* sreq = static_cast< StartRequest* >( currentRequest->value );
	switch( type )
	{
	case SimGsmATResponseParsers::ConnectOk:
	{
		dereserveLink( sreq->linkId );
		assert( linkDesc[sreq->linkId].socket == nullptr );
		chSysLock();
		linkDesc[sreq->linkId].socket = sreq->socket;
		static_cast< SimGsmUdpSocket* >( sreq->socket )->connectHelpS( sreq->linkId );
		chBSemSignalI( &sreq->semaphore );
		chSchRescheduleS();
		chSysUnlock();
		nextRequest();
		break;
	}
	case SimGsmATResponseParsers::Ok: // CLPORT response
	{
		if( linkDesc[sreq->linkId].socket )
		{
			dereserveLink( sreq->linkId );
			chBSemSignal( &sreq->semaphore );
			nextRequest();
			break;
		}
		lexicalAnalyzer.setState( CIPSTART_ANALYZER_STATE );
		sendCommand( printCIPSTART( sreq->linkId, SocketType::Udp, sreq->remoteAddress, sreq->remotePort ) );
		break;
	}
	case SimGsmATResponseParsers::AlreadyConnect:
	{
		chSysLock();
		requestList.pushFront( currentRequest );
		chSysUnlock();
		nextRequest();
		break;
	}
	case SimGsmATResponseParsers::ConnectFail:
		dereserveLink( sreq->linkId );
		chSysLock();
		static_cast< SimGsmUdpSocket* >( sreq->socket )->sError = SocketError::ConnectionRefusedError;
		sreq->socket->eventSource()->broadcastFlagsI( ( eventflags_t )SocketEventFlag::Error );
		chBSemSignalI( &sreq->semaphore );
		chSchRescheduleS();
		chSysUnlock();
		nextRequest();
		break;
	case SimGsmATResponseParsers::Error: // CLPORT response	
	case SimGsmATResponseParsers::CMEError:
	{
		dereserveLink( sreq->linkId );
		chSysLock();
		static_cast< SimGsmUdpSocket* >( sreq->socket )->sError = SocketError::NetworkError;
		sreq->socket->eventSource()->broadcastFlagsI( ( eventflags_t )SocketEventFlag::Error );
		chBSemSignalI( &sreq->semaphore );
		chSchRescheduleS();
		chSysUnlock();
		nextRequest();
		break;
	}
	default:
		assert( false );
		break;
	}
}

void SimGsm::closeRequestHandler( LexicalAnalyzer::ParsingResult* res )
{
	ParserResultType type = ( ParserResultType )res->type();
	CloseRequest* creq = static_cast< CloseRequest* >( currentRequest->value );
	switch( type )
	{
	case SimGsmATResponseParsers::CloseOk:
	{
		chSysLock();
		linkDesc[static_cast< SimGsmUdpSocket* >( creq->socket )->linkId].socket = nullptr;
		static_cast< SimGsmUdpSocket* >( creq->socket )->closeHelpS( SocketError::NoError );
		chBSemSignalI( &creq->semaphore );
		chSchRescheduleS();
		chSysUnlock();
		nextRequest();
		break;
	}
	case SimGsmATResponseParsers::Error:
	{
		crash();
		break;
	}
	default:
		assert( false );
		break;
	}
}

void SimGsm::serverRequestHandler( LexicalAnalyzer::ParsingResult* res )
{
	ParserResultType type = ( ParserResultType )res->type();
	ServerRequest* req = static_cast< ServerRequest* >( currentRequest->value );
	switch( type )
	{
	case SimGsmATResponseParsers::ServerOk:
	{
		server->listening = true;
		chBSemSignal( &req->semaphore );
		nextRequest();
		break;
	}
	case SimGsmATResponseParsers::ServerClose:
	{
		server->listening = false;
		chBSemSignal( &req->semaphore );
		nextRequest();
		break;
	}
	case SimGsmATResponseParsers::Error:
	case SimGsmATResponseParsers::CMEError:
	{
		if( req->port == 0 ) // SERVER CLOSE request
			crash();
		else
		{
			chSysLock();
			server->sError = SocketError::NetworkError;
			server->eventSource()->broadcastFlagsI( ( eventflags_t )SocketEventFlag::Error );
			chBSemSignalI( &req->semaphore );
			chSchRescheduleS();
			chSysUnlock();
			nextRequest();
		}
		break;
	}
	default:
		assert( false );
		break;
	}
}

void SimGsm::atRequestHandler( LexicalAnalyzer::ParsingResult* res )
{
	atReq.use = false;
	nextRequest();
}

bool SimGsm::addRequest( RequestNode* requestNode )
{
	chSysLock();
	volatile bool res = addRequestS( requestNode );
	chSchRescheduleS();
	chSysUnlock();

	return res;
}

bool SimGsm::addRequestS( RequestNode* requestNode )
{
	if( mStatus == ModemStatus::Initializing || mStatus == ModemStatus::Stopped )
		return false;
	requestList.pushBack( requestNode );
	if( currentRequest == nullptr )
		innerEventSource.broadcastFlagsI( InnerEventFlag::NewRequestEventFlag );

	return true;
}

void SimGsm::nextRequest()
{
Next:
	chSysLock();
	currentRequest = requestList.popFront();
	chSysUnlock();
	if( !currentRequest )
	{
		lexicalAnalyzer.setState( 0 );
		return;
	}
	switch( currentRequest->value->type )
	{
	case Request::Type::Send:
	{
		SendRequest* sreq = static_cast< SendRequest* >( currentRequest->value );
		if( sreq->socket->type() == SocketType::Udp )
		{
			SimGsmUdpSocket* udpSocket = static_cast< SimGsmUdpSocket* >( sreq->socket );
			if( udpSocket->cAddr == sreq->remoteAddress && udpSocket->cPort == sreq->remotePort )
			{
				sendReqState = SendRequestState::SendInit;
				lexicalAnalyzer.setState( CIPSEND_ANALYZER_STATE );
				sendCommand( printCIPSEND( udpSocket->linkId, sreq->dataSize[0] + sreq->dataSize[1] ) );
			}
			else
			{
				sendReqState = SendRequestState::SetRemoteAddress;
				lexicalAnalyzer.setState( UDPMODE_ANALYZER_STATE );
				sendCommand( printCIPUDPMODE2( udpSocket->linkId, sreq->remoteAddress, sreq->remotePort ) );
			}
		}
		else
		{
			SimGsmTcpSocket* tcpSocket = static_cast< SimGsmTcpSocket* >( sreq->socket );
			sendReqState = SendRequestState::SendInit;
			lexicalAnalyzer.setState( CIPSEND_ANALYZER_STATE );
			sendCommand( printCIPSEND( tcpSocket->linkId, sreq->dataSize[0] + sreq->dataSize[1] ) );
		}
		break;
	}
	case Request::Type::Start:
	{
		StartRequest* sreq = static_cast< StartRequest* >( currentRequest->value );
		sreq->linkId = reserveLink();
		if( sreq->linkId == -1 )
		{
			chSysLock();
			static_cast< SimGsmUdpSocket* >( sreq->socket )->sError = SocketError::SocketResourceError;
			sreq->socket->eventSource()->broadcastFlagsI( ( eventflags_t )SocketEventFlag::Error );
			chBSemSignalI( &sreq->semaphore );
			chSchRescheduleS();
			chSysUnlock();
			goto Next;
		}
		if( sreq->socket->type() == SocketType::Udp )
		{
			lexicalAnalyzer.setState( CLPORT_ANALYZER_STATE );
			sendCommand( printCLPORT( sreq->linkId, sreq->localPort ) );
		}
		else
		{
			lexicalAnalyzer.setState( CIPSTART_ANALYZER_STATE );
			sendCommand( printCIPSTART( sreq->linkId, SocketType::Tcp, sreq->remoteAddress, sreq->remotePort ), TIME_S2I( 6 ) );
		}
		break;
	}
	case Request::Type::Close:
	{
		CloseRequest* creq = static_cast< CloseRequest* >( currentRequest->value );
		lexicalAnalyzer.setState( CIPCLOSE_ANALYZER_STATE );
		sendCommand( printCIPCLOSE( static_cast< SimGsmUdpSocket* >( creq->socket )->linkId ) );
		break;
	}
	case Request::Type::At:
	{
		lexicalAnalyzer.setState( AT_ANALYZER_STATE );
		sendCommand( printAT() );
		break;
	}
	case Request::Type::Server:
	{
		ServerRequest* sreq = static_cast< ServerRequest* >( currentRequest->value );
		lexicalAnalyzer.setState( CIPSERVER_ANALYZER_STATE );
		sendCommand( printCIPSERVER( sreq->port ) );
		break;
	}
	default:
		break;
	}
}

void SimGsm::completePacketTransfer( bool errorFlag )
{
	chSysLock();
	SendRequest* req = static_cast< SendRequest* >( currentRequest->value );
	SimGsmSocketBase* base = static_cast< SimGsmUdpSocket* >( req->socket );
	if( base->outBuffer )
	{
		if( req->socket->type() == SocketType::Udp )
		{
			base->outBuffer->read( nullptr, sizeof( SimGsmUdpSocket::Header ) + req->dataSize[0] + req->dataSize[1] );
			if( base->outBuffer->readAvailable() >= sizeof( SimGsmUdpSocket::Header ) )
			{
				SimGsmUdpSocket::Header header;
				Utility::copy( reinterpret_cast< uint8_t* >( &header ), base->outBuffer->begin(), sizeof( header ) );
				if( base->outBuffer->readAvailable() >= sizeof( SimGsmUdpSocket::Header ) + header.datagramSize )
				{
					req->remoteAddress = header.remoteAddress;
					req->remotePort = header.remotePort;
					uint8_t* data[2];
					uint32_t dataSize[2];
					ringToLinearArrays( base->outBuffer->begin() + sizeof( SimGsmUdpSocket::Header ), header.datagramSize, data, dataSize, data + 1, dataSize + 1 );
					req->data[0] = data[0];
					req->data[1] = data[1];
					req->dataSize[0] = ( uint16_t )dataSize[0];
					req->dataSize[1] = ( uint16_t )dataSize[1];
					moveSendRequestToBackS();
				}
				else
					req->use = false;
			}
			else
			{
				req->use = false;
				if( base->outBuffer->readAvailable() == 0 )
					base->eSource.broadcastFlagsI( ( eventflags_t )SocketEventFlag::OutputEmpty );
			}
		}
		else
		{
			base->outBuffer->read( nullptr, req->dataSize[0] + req->dataSize[1] );
			uint32_t size = base->outBuffer->readAvailable();
			if( size )
			{
				if( size > MAX_TCP_PACKET_SIZE )
					size = MAX_TCP_PACKET_SIZE;

				uint8_t* data[2];
				uint32_t dataSize[2];
				ringToLinearArrays( base->outBuffer->begin(), size, data, dataSize, data + 1, dataSize + 1 );
				req->data[0] = data[0];
				req->data[1] = data[1];
				req->dataSize[0] = ( uint16_t )dataSize[0];
				req->dataSize[1] = ( uint16_t )dataSize[1];
				moveSendRequestToBackS();
			}
			else
			{
				req->use = false;
				base->eSource.broadcastFlagsI( ( eventflags_t )SocketEventFlag::OutputEmpty );
			}
		}
	}
	else
	{
		if( errorFlag )
			chBSemResetI( &req->semaphore, false );
		else
			chBSemSignalI( &req->semaphore );
	}
	chSchRescheduleS();
	chSysUnlock();
}

void SimGsm::moveSendRequestToBackS()
{
	SendRequest* req = static_cast< SendRequest* >( currentRequest->value );
	NanoList< Request* >::Iterator i;
	const auto end = requestList.end();
	if( static_cast< SimGsmUdpSocket* >( req->socket )->sState == SocketState::Closing )
	{
		i = requestList.begin();
		while( i != end )
		{
			if( ( *i )->type == Request::Type::Close && static_cast< CloseRequest* >( *i )->socket == req->socket )
				break;
			++i;
		}
	}
	else
		i = end;
	requestList.insert( i, currentRequest );
}

bool SimGsm::openUdpSocket( AbstractUdpSocket* socket, uint16_t localPort )
{
	StartRequest req( socket, localPort, IpAddress( 8, 8, 8, 8 ), 8888 );
	RequestNode reqNode( &req );
	if( !addRequest( &reqNode ) )
		return false;
	chBSemWait( &req.semaphore );

	return socket->isOpen();
}

bool SimGsm::openTcpSocket( AbstractTcpSocket* socket, IpAddress remoteAddress, uint16_t remotePort )
{
	StartRequest req( socket, 0, remoteAddress, remotePort );
	RequestNode reqNode( &req );
	if( !addRequest( &reqNode ) )
		return false;
	chBSemWait( &req.semaphore );

	return socket->isOpen();
}

uint32_t SimGsm::sendSocketData( AbstractUdpSocket* socket, const uint8_t* data, uint16_t size, IpAddress remoteAddress, uint16_t remotePort )
{
	if( size > MAX_UDP_PACKET_SIZE || size == 0 )
		return 0;

	uint32_t bs = 0;
	SimGsmUdpSocket* udpSocket = static_cast< SimGsmUdpSocket* >( socket );
	if( udpSocket->outBuffer )
	{
		SendRequest& sreq = udpSocket->ssend->req;
		chSysLock();
		if( udpSocket->linkId != -1 &&
			udpSocket->outBuffer->write( reinterpret_cast< uint8_t* >( &remoteAddress ), sizeof( IpAddress ), TIME_INFINITE ) &&
			udpSocket->outBuffer->write( reinterpret_cast< uint8_t* >( &remotePort ), 2, TIME_INFINITE ) &&
			udpSocket->outBuffer->write( reinterpret_cast< uint8_t* >( &size ), 2, TIME_INFINITE ) &&
			udpSocket->outBuffer->write( data, size, TIME_INFINITE ) &&
			!sreq.use )
		{
			sreq.socket = udpSocket;
			sreq.remoteAddress = remoteAddress;
			sreq.remotePort = remotePort;
			sreq.use = true;

			uint8_t* data[2];
			uint32_t dataSize[2];
			ringToLinearArrays( udpSocket->outBuffer->begin() + sizeof( SimGsmUdpSocket::Header ), size, data, dataSize, data + 1, dataSize + 1 );
			sreq.data[0] = data[0];
			sreq.data[1] = data[1];
			sreq.dataSize[0] = ( uint16_t )dataSize[0];
			sreq.dataSize[1] = ( uint16_t )dataSize[1];

			if( addRequestS( &udpSocket->ssend->node ) )
				bs = size;
		}
		chSysUnlock();
	}
	else
	{
		chSysLock();
		if( udpSocket->linkId != -1 )
		{
			SendRequest sreq;
			RequestNode node( &sreq );
			sreq.socket = udpSocket;
			sreq.remoteAddress = remoteAddress;
			sreq.remotePort = remotePort;

			sreq.data[0] = data;
			sreq.data[1] = nullptr;
			sreq.dataSize[0] = size;
			sreq.dataSize[1] = 0;

			if( addRequestS( &node ) && chBSemWaitS( &sreq.semaphore ) == MSG_OK )
				bs = size;
		}
		chSysUnlock();
	}

	return bs;
}

uint32_t SimGsm::sendSocketData( AbstractTcpSocket* socket, const uint8_t* data, uint16_t size )
{
	if( size == 0 )
		return 0;

	uint32_t bs = 0;
	SimGsmTcpSocket* tcpSocket = static_cast< SimGsmTcpSocket* >( socket );
	if( tcpSocket->outBuffer )
	{
		SendRequest& sreq = tcpSocket->ssend->req;
		chSysLock();
		if( tcpSocket->linkId != -1 &&
			tcpSocket->outBuffer->write( data, size, TIME_INFINITE ) &&
			!sreq.use )
		{
			sreq.socket = tcpSocket;
			sreq.use = true;

			if( size > MAX_TCP_PACKET_SIZE )
				size = MAX_TCP_PACKET_SIZE;
			uint8_t* data[2];
			uint32_t dataSize[2];
			ringToLinearArrays( tcpSocket->outBuffer->begin(), size, data, dataSize, data + 1, dataSize + 1 );
			sreq.data[0] = data[0];
			sreq.data[1] = data[1];
			sreq.dataSize[0] = ( uint16_t )dataSize[0];
			sreq.dataSize[1] = ( uint16_t )dataSize[1];

			if( addRequestS( &tcpSocket->ssend->node ) )
				bs = size;
		}
		chSysUnlock();
	}
	else
	{
		chSysLock();
		SendRequest sreq;
		RequestNode node( &sreq );
		sreq.socket = tcpSocket;

		uint16_t leftSize = size;
		while( leftSize && tcpSocket->linkId != -1 )
		{
			uint16_t stepSize = leftSize;
			if( stepSize > MAX_TCP_PACKET_SIZE )
				stepSize = MAX_TCP_PACKET_SIZE;

			sreq.data[0] = data;
			sreq.data[1] = nullptr;
			sreq.dataSize[0] = stepSize;
			sreq.dataSize[1] = 0;

			if( !addRequestS( &node ) || chBSemWaitS( &sreq.semaphore ) != MSG_OK )
				break;
			leftSize -= stepSize;
			data += stepSize;
		}

		bs = size - leftSize;
		chSysUnlock();
	}

	return bs;
}

void SimGsm::closeSocket( AbstractSocket* socket )
{
	CloseRequest req( socket );
	RequestNode reqNode( &req );
	chSysLock();
	if( static_cast< SimGsmUdpSocket* >( socket )->linkId != -1 && addRequestS( &reqNode ) )
	{
		static_cast< SimGsmUdpSocket* >( socket )->sState = SocketState::Closing;
		chBSemWaitS( &req.semaphore );
	}
	chSysUnlock();
}

bool SimGsm::serverStart( uint16_t port )
{
	ServerRequest req( port );
	RequestNode reqNode( &req );
	if( !addRequest( &reqNode ) )
		return false;
	chBSemWait( &req.semaphore );

	return server->listening;
}

void SimGsm::serverStop()
{
	ServerRequest req( 0 );
	RequestNode reqNode( &req );
	chSysLock();
	if( server->listening && addRequestS( &reqNode ) )
		chBSemWaitS( &req.semaphore );
	chSysUnlock();
}

void SimGsm::sendCommand( uint32_t len, sysinterval_t timeout )
{
	chVTSet( &responseTimer, timeout, timeoutCallback, this );
	send( ( const uint8_t* )str, len, nullptr, 0 );
}

void SimGsm::send( const uint8_t* dataFirst, uint32_t sizeFirst, const uint8_t* dataSecond, uint32_t sizeSecond )
{
	assert( !dataSend[0] && !dataSend[1] );

	dataSend[0] = dataFirst;
	dataSendSize[0] = sizeFirst;
	dataSend[1] = dataSecond;
	dataSendSize[1] = sizeSecond;

	uint32_t done = usart->outputBuffer()->write( dataSend[0], dataSendSize[0] );
	if( done == dataSendSize[0] )
	{
		dataSend[0] = nullptr;
		dataSendSize[0] = 0;
	}
	else
	{
		dataSend[0] += done;
		dataSendSize[0] -= done;
		return;
	}

	if( !dataSend[1] )
		return;
	done = usart->outputBuffer()->write( dataSend[1], dataSendSize[1] );
	if( done == dataSendSize[1] )
	{
		dataSend[1] = nullptr;
		dataSendSize[1] = 0;
	}
	else
	{
		dataSend[1] += done;
		dataSendSize[1] -= done;
	}
}

void SimGsm::nextSend()
{
	if( dataSend[0] )
	{
		uint32_t done = usart->outputBuffer()->write( dataSend[0], dataSendSize[0] );
		if( done == dataSendSize[0] )
		{
			dataSend[0] = nullptr;
			dataSendSize[0] = 0;
		}
		else
		{
			dataSend[0] += done;
			dataSendSize[0] -= done;
			return;
		}
	}

	if( dataSend[1] )
	{
		uint32_t done = usart->outputBuffer()->write( dataSend[1], dataSendSize[1] );
		if( done == dataSendSize[1] )
		{
			dataSend[1] = nullptr;
			dataSendSize[1] = 0;
		}
		else
		{
			dataSend[1] += done;
			dataSendSize[1] -= done;
		}
	}
}

void SimGsm::closeAllS()
{
	if( currentRequest )
		chBSemResetI( &currentRequest->value->semaphore, false );
	for( currentRequest = requestList.popFront(); currentRequest; currentRequest = requestList.popFront() )
		chBSemResetI( &currentRequest->value->semaphore, false );

	for( int i = 0; i < SIMGSM_SOCKET_LIMIT; ++i )
	{
		linkDesc[i].reserved = false;
		if( linkDesc[i].socket )
		{
			static_cast< SimGsmUdpSocket* >( linkDesc[i].socket )->closeHelpS( SocketError::NetworkError );
			linkDesc[i].socket = nullptr;
		}
	}

	if( server && server->listening )
		server->closeHelpS( SocketError::NetworkError );

	chVTResetI( &responseTimer );
	chVTResetI( &modemPingTimer );
	atReq.use = false;
}

void SimGsm::crash()
{
	lexicalAnalyzer.setState( 0 );
	lexicalAnalyzer.setUniversumStateEnabled( false );
	crashFlag = true;
}

void SimGsm::shutdown()
{
	ip = IpAddress();
#ifdef SIMGSM_USE_PWRKEY_PIN
	do
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
	} while( !pwrDown );
#else
	enablePin.off();
#endif
}

void SimGsm::timeoutCallback( void* p )
{
	chSysLockFromISR();
	reinterpret_cast< SimGsm* >( p )->innerEventSource.broadcastFlagsI( InnerEventFlag::TimeoutEventFlag );
	chSysUnlockFromISR();
}

void SimGsm::modemPingCallback( void* p )
{
	chSysLockFromISR();
	reinterpret_cast< SimGsm* >( p )->innerEventSource.broadcastFlagsI( InnerEventFlag::ModemPingEventFlag );
	chSysUnlockFromISR();
}

int SimGsm::reserveLink()
{
	for( int i = SIMGSM_SOCKET_LIMIT - 1; i >= 0; --i )
	{
		if( !linkDesc[i].socket && !linkDesc[i].reserved )
		{
			linkDesc[i].reserved = true;
			return i;
		}
	}

	return -1;
}

void SimGsm::dereserveLink( int id )
{
	linkDesc[id].reserved = false;
}

int SimGsm::printCSTT()
{
	const char* apn = this->apn;
	memcpy( str, "AT+CSTT=\"", 9 );
	size_t len = strlen( apn );
	memcpy( str + 9, apn, len );
	memcpy( str + 9 + len, "\"\r\n", 3 );

	return len + 12;
}

int SimGsm::printCPIN()
{
	uint32_t pinCode = this->pinCode;
	memcpy( str, "AT+CPIN=", 8 );
	char* cmd = Utility::printInt( str + 8, pinCode );
	*cmd++ = '\r', *cmd++ = '\n';

	return cmd - str;
}

int SimGsm::printCLPORT( int linkId, uint16_t port )
{
	memcpy( str, "AT+CLPORT=0,\"UDP\",", 18 );
	str[10] = linkId + '0';
	char* cmd = Utility::printInt( str + 18, port );
	*cmd++ = '\r', *cmd++ = '\n';

	return cmd - str;
}

int SimGsm::printCIPCLOSE( int linkId )
{
	memcpy( str, "AT+CIPCLOSE=0,1\r\n", 17 );
	str[12] = linkId + '0';

	return 17;
}

int SimGsm::printCIPUDPMODE2( int linkId, IpAddress addr, uint16_t port )
{
	memcpy( str, "AT+CIPUDPMODE=0,2,\"", 19 );
	str[14] = linkId + '0';
	char* cmd = str + 19;
	for( int i = 3; i >= 0; --i )
	{
		cmd = printInt( cmd, addr.addr[i] );
		*cmd++ = '.';
	}
	*( cmd - 1 ) = '\"', *cmd++ = ',';
	cmd = printInt( cmd, port );
	*cmd++ = '\r', *cmd++ = '\n';

	return cmd - str;
}

int SimGsm::printCIPSERVER( uint16_t port )
{
	if( port )
	{
		memcpy( str, "AT+CIPSERVER=1,", 15 );
		char* cmd = printInt( str + 15, port );
		*cmd++ = '\r', *cmd++ = '\n';

		return cmd - str;
	}

	memcpy( str, "AT+CIPSERVER=0\r\n", 16 );
	return 16;
}

int SimGsm::printAT()
{
	memcpy( str, "AT\r\n", 4 );

	return 4;
}

int SimGsm::printCIPSTART( int linkId, SocketType type, IpAddress addr, uint16_t port )
{
	if( type == SocketType::Udp )
		memcpy( str, "AT+CIPSTART=0,\"UDP\",\"", 21 );
	else
		memcpy( str, "AT+CIPSTART=0,\"TCP\",\"", 21 );
	str[12] = linkId + '0';
	char* cmd = str + 21;
	for( int i = 3; i >= 0; --i )
	{
		cmd = printInt( cmd, addr.addr[i] );
		*cmd++ = '.';
	}
	*( cmd - 1 ) = '\"', *cmd++ = ',', *cmd++ = '\"';
	cmd = printInt( cmd, port );
	*cmd++ = '\"', *cmd++ = '\r', *cmd++ = '\n';

	return cmd - str;
}

int SimGsm::printCIPSEND( int linkId, uint32_t dataSize )
{
	memcpy( str, "AT+CIPSEND=0,", 13 );
	str[11] = linkId + '0';
	char* cmd = printInt( ( char* )str + 13, dataSize );
	*cmd++ = '\r', *cmd++ = '\n';

	return cmd - str;
}