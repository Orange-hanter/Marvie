#include "AbstractPppModem.h"
#include <string.h>

#define INPUT_DELAY TIME_MS2I( 1 )

AbstractPppModem::AbstractPppModem( uint32_t stackSize ) : Modem( stackSize )
{
	memset( &pppNetif, 0, sizeof( pppNetif ) );
	memset( buffer, 0, sizeof( buffer ) );
	LOCK_TCPIP_CORE();
	pppPcb = pppos_create( &pppNetif, outputCallback, linkStatusCallback, this );
	netif_set_status_callback( &pppNetif, netifStatusCallback );
	netif_get_client_data( &pppNetif, LWIP_NETIF_CLIENT_DATA_INDEX_MAX ) = this;
	UNLOCK_TCPIP_CORE();
	attemptNumber = 0;
	len = 0;
	chVTObjectInit( &timer );
}

AbstractPppModem::~AbstractPppModem()
{
	stopModem();
	waitForStateChange();
	LOCK_TCPIP_CORE();
	pppPcb->ctx_cb = nullptr;
	ppp_free( pppPcb );
	netif_remove( &pppNetif );
	UNLOCK_TCPIP_CORE();
}

bool AbstractPppModem::startModem( tprio_t prio /*= NORMALPRIO */ )
{
	if( pppPcb == nullptr )
		return false;
	return Modem::startModem( prio );
}

void AbstractPppModem::setAsDefault()
{
	LOCK_TCPIP_CORE();
	netif_set_default( &pppNetif );
	UNLOCK_TCPIP_CORE();
}

void AbstractPppModem::main()
{
	err_t err;
	EventListener ioListener;
	mError = ModemError::NoError;
	LowLevelError llErr;
	attemptNumber = 0;

Start:
	if( ++attemptNumber > 3 )
	{
		int n = attemptNumber - 3;
		if( n > 4 )
			n = 4;
		sysinterval_t timeout = TIME_S2I( 2 * 60 * n );
		while( timeout )
		{
			chThdSleepMilliseconds( 200 );
			timeout -= TIME_MS2I( 200 );
			if( mState == ModemState::Stopping )
				goto End;
		}
	}
	llErr = lowLevelStart();
	if( llErr != LowLevelError::NoError )
		goto End;
	chEvtGetAndClearEvents( ~StopRequestEvent );
	LOCK_TCPIP_CORE();
	err = ppp_connect( pppPcb, 0 );
	UNLOCK_TCPIP_CORE();
	if( err != ERR_OK )
		goto End;

	usart->eventSource().registerMaskWithFlags( &ioListener, IOEvent, CHN_INPUT_AVAILABLE );
	chVTSet( &timer, INPUT_DELAY, timerCallback, this );

	while( mState != ModemState::Stopping )
	{
		eventmask_t em = chEvtWaitAny( ALL_EVENTS );
		if( em & StopRequestEvent )
			break;
		if( em & LinkDownEvent )
		{
			chSysLock();
			if( mState != ModemState::Stopping )
			{
				setModemStateS( ModemState::Initializing );				
				chVTResetI( &timer );
				chEvtGetAndClearEventsI( IOEvent | TimerEvent );
				chSchRescheduleS();
				chSysUnlock();
				ioListener.unregister();
				lowLevelStop();
				len = 0;
				goto Start;
			}
			chSysUnlock();
		}
		if( em & LinkUpEvent )
		{
			chSysLock();
			if( mState == ModemState::Initializing )
			{
				setModemStateS( ModemState::Working );
				chSchRescheduleS();
			}
			chSysUnlock();
			attemptNumber = 0;
		}
		if( em & IOEvent )
		{
			while( true )
			{
				len += usart->read( buffer + len, sizeof( buffer ) - len, TIME_IMMEDIATE );
				if( len == sizeof( buffer ) )
				{
					pppos_input_tcpip( pppPcb, buffer, sizeof( buffer ) );
					len = 0;
				}
				else
					break;
			}
			chSysLock();
			chVTSetI( &timer, INPUT_DELAY, timerCallback, this );
			chEvtGetAndClearEventsI( TimerEvent );
			chSysUnlock();
			continue;
		}
		if( em & TimerEvent )
		{
			if( len )
			{
				pppos_input_tcpip( pppPcb, buffer, len );
				len = 0;
			}
		}
	}

	chVTReset( &timer );
	ioListener.unregister();

	LOCK_TCPIP_CORE();
	ppp_close( pppPcb, 1 );
	UNLOCK_TCPIP_CORE();
	lowLevelStop();

End:
	chSysLock();
	setModemStateS( ModemState::Stopped );
	if( llErr != LowLevelError::NoError )
	{
		if( llErr == LowLevelError::TimeoutError )
			setModemErrorS( ModemError::TimeoutError );
		else if( llErr == LowLevelError::SimCardNotInsertedError || llErr == LowLevelError::PinCodeError )
			setModemErrorS( ModemError::AuthenticationError );
		else
			setModemErrorS( ModemError::UnknownError );
	}
	len = 0;
}

u32_t AbstractPppModem::outputCallback( ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx )
{
	if( ctx == nullptr )
		return 0;
	AbstractPppModem* modem = ( AbstractPppModem* )ctx;
	return modem->usart->write( data, len, TIME_IMMEDIATE );
}

void AbstractPppModem::linkStatusCallback( ppp_pcb *pcb, int err_code, void *ctx )
{
	AbstractPppModem* modem = ( AbstractPppModem* )ctx;
	if( err_code == PPPERR_NONE )
		modem->signalEvents( LinkUpEvent );
	else
		modem->signalEvents( LinkDownEvent );
}

void AbstractPppModem::netifStatusCallback( netif *nif )
{
	uint32_t addr = netif_ip4_addr( nif )->addr;
	addr = ( addr & 0xFF ) << 24 | ( ( addr >> 8 ) & 0xFF ) << 16 | ( ( addr >> 16 ) & 0xFF ) << 8 | ( ( addr >> 24 ) & 0xFF );
	reinterpret_cast< AbstractPppModem* >( netif_get_client_data( nif, LWIP_NETIF_CLIENT_DATA_INDEX_MAX ) )->setNetworkAddress( IpAddress( addr ) );
}

void AbstractPppModem::timerCallback( void* p )
{
	AbstractPppModem* modem = ( AbstractPppModem* )p;
	chSysLockFromISR();
	modem->signalEventsI( TimerEvent );
	chSysUnlockFromISR();
}