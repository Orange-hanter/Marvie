#include "PingService.h"
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/raw.h"

PingService* PingService::inst = nullptr;

PingService::PingService() : BaseDynamicThread( 1024 )
{
	sState = State::Stopped;
	sError = Error::NoError;
	chThdQueueObjectInit( &waitingQueue );
	chVTObjectInit( &timer );
	pongTimeout = TIME_MS2I( 4000 );
	seqNum = 0;
	conn = nullptr;
	recvCount = 0;
}

PingService::~PingService()
{
	stopService();
	waitForStopped();
}

PingService* PingService::instance()
{
	if( inst )
		return inst;
	inst = new PingService;
	return inst;
}

bool PingService::startService( tprio_t prio /*= NORMALPRIO */ )
{
	if( sState != State::Stopped )
		return false;

	sState = State::Working;
	sError = Error::NoError;
	extEventSource.broadcastFlags( ( eventflags_t )Event::StateChanged );
	start( prio );
	return true;
}

void PingService::stopService()
{
	chSysLock();
	if( sState == State::Stopped || sState == State::Stopping )
	{
		chSysUnlock();
		return;
	}
	sState = State::Stopping;
	extEventSource.broadcastFlagsI( ( eventflags_t )Event::StateChanged );
	chEvtSignalI( thread_ref, StopRequestEvent );
	chSchRescheduleS();
	chSysUnlock();
}

bool PingService::waitForStopped( sysinterval_t timeout /*= TIME_INFINITE */ )
{
	msg_t msg = MSG_OK;
	chSysLock();
	if( sState == State::Stopping )
		msg = chThdEnqueueTimeoutS( &waitingQueue, timeout );
	chSysUnlock();

	return msg == MSG_OK;
}

PingService::State PingService::state()
{
	return sState;
}

PingService::Error PingService::error()
{
	return sError;
}

int PingService::ping( IpAddress addr, uint32_t count /*= 1*/, TimeMeasurement* pTM /*= nullptr */, uint32_t delayMs /*= 1000*/ )
{
	Request req( addr );
	NanoList< Request* >::Node node( &req );
	if( pTM )
		pTM->minMs = pTM->maxMs = pTM->avgMs = -1;
	uint32_t sumMs = 0;
	uint32_t nOk = 0;
	sysinterval_t delay = TIME_MS2I( delayMs );

	while( count )
	{
		chSysLock();
		mutex.lockS();
		if( sState != State::Working )
		{
			chSysUnlock();
			if( pTM && nOk )
				pTM->avgMs = sumMs / nOk;
			return nOk;
		}
		reqList.pushBack( &node );
		chEvtSignalI( thread_ref, PingRequestEvent );
		mutex.unlockS();
		chBSemWaitS( &req.sem );
		chSysUnlock();

		if( req.result )
		{
			++nOk;
			int32_t ms = TIME_I2MS( req.interval );
			sumMs += ms;
			if( pTM )
			{
				if( ms < pTM->minMs || pTM->minMs == -1 )
					pTM->minMs = ms;
				if( ms > pTM->maxMs )
					pTM->maxMs = ms;
			}
			chThdSleep( delay );
		}
		else
		{
			if( pongTimeout < delay )
				chThdSleep( delay - pongTimeout );
		}
		--count;
	}

	if( pTM && nOk )
		pTM->avgMs = sumMs / nOk;
	return nOk;
}

void PingService::setPongTimeout( uint32_t timeoutMs )
{
	if( timeoutMs > MaxPongTimeoutMs )
		timeoutMs = MaxPongTimeoutMs;
	pongTimeout = TIME_MS2I( timeoutMs );
}

chibios_rt::EvtSource* PingService::eventSource()
{
	return &extEventSource;
}

void PingService::main()
{
	recvCount = 0;
	conn = netconn_new_with_proto_and_callback( NETCONN_RAW, IP_PROTO_ICMP, netconnCallback );
	if( conn == nullptr )
	{
		sError = Error::MemoryError;
		goto End;
	}
	netconn_set_recvtimeout( conn, 100 );

	while( sState == State::Working )
	{
		eventmask_t em = chEvtWaitAny( ALL_EVENTS );
		if( em & StopRequestEvent )
			break;
		if( em & NetconnErrorEvent )
		{
			sError = Error::NetworkError;
			break;
		}
		if( em & NetconnRecvEvent )
		{
			while( recvCount )
			{
				err_t err;
				netbuf* buf;
				if( ( err = netconn_recv( conn, &buf ) ) == ERR_OK )
				{
					if( IP_IS_V4_VAL( buf->addr ) )
					{
						uint8_t* data;
						u16_t len;
						netbuf_data( buf, ( void** )&data, &len );

						ip_hdr* iphdr = ( ip_hdr* )data;
						icmp_echo_hdr* iecho = ( icmp_echo_hdr* )( data + ( IPH_HL( iphdr ) * 4 ) );

						if( iecho->id == lwip_htons( PingID ) )
						{
							u16_t seq = lwip_ntohs( iecho->seqno );
							mutex.lock();
							for( auto i = reqList.begin(); i != reqList.end(); ++i )
							{
								auto req = *i;
								if( req->processing && req->seqNum == seq )
								{
									reqList.remove( i );
									req->interval = chVTTimeElapsedSinceX( req->sendingTime );
									req->result = ICMPH_TYPE( iecho ) == ICMP_ER;
									req->processing = false;
									chBSemSignal( &req->sem );
									break;
								}
							}
							if( reqList.isEmpty() )
								chVTReset( &timer );
							mutex.unlock();
						}
					}
					netbuf_delete( buf );
				}
				else
				{
					if( err == ERR_MEM )
						sError = Error::MemoryError;
					else
						sError = Error::NetworkError;
					goto End;
				}
			}
		}
		if( em & TimerEvent )
		{
			mutex.lock();
			for( auto i = reqList.begin(); i != reqList.end(); )
			{
				auto req = *i;
				if( req->processing )
				{
					if( chVTTimeElapsedSinceX( req->sendingTime ) >= pongTimeout )
					{
						reqList.remove( i++ );
						req->result = false;
						req->processing = false;
						chBSemSignal( &req->sem );
						continue;
					}
				}
				++i;
			}
			if( reqList.isEmpty() )
				chVTReset( &timer );
			mutex.unlock();
		}
		if( em & PingRequestEvent )
		{
			mutex.lock();
			for( auto i = reqList.begin(); i != reqList.end(); )
			{
				auto req = *i;
				if( !req->processing )
				{
					auto err = sendPing( req->addr );
					if( err != ERR_OK )
					{
						reqList.remove( i++ );
						req->result = false;
						chBSemSignal( &req->sem );
						continue;
					}

					req->sendingTime = chVTGetSystemTimeX();
					req->seqNum = seqNum;
					req->processing = true;
				}
				++i;
			}
			if( !chVTIsArmed( &timer ) )
				chVTSet( &timer, TIME_MS2I( 100 ), timerCallback, nullptr );
			mutex.unlock();
		}
	}

End:
	chVTReset( &timer );
	sState = State::Stopping;
	if( conn )
		netconn_delete( conn ), conn = nullptr;
	mutex.lock();
	for( auto i = reqList.begin(); i != reqList.end(); )
	{
		auto req = *i;
		reqList.remove( i++ );
		req->result = false;
		req->processing = false;
		chBSemSignal( &req->sem );
	}
	mutex.unlock();

	chSysLock();
	sState = State::Stopped;
	if( sError != Error::NoError )
		extEventSource.broadcastFlagsI( ( eventflags_t )Event::Error | ( eventflags_t )Event::StateChanged );
	else
		extEventSource.broadcastFlagsI( ( eventflags_t )Event::StateChanged );
	chThdDequeueAllI( &waitingQueue, MSG_OK );
	exitS( MSG_OK );
}

err_t PingService::sendPing( IpAddress address )
{
	constexpr size_t pingPacketSize = sizeof( icmp_echo_hdr ) + PingLoadSize;
	uint8_t data[pingPacketSize];
	icmp_echo_hdr* iecho = reinterpret_cast< icmp_echo_hdr* >( data );

	ICMPH_TYPE_SET( iecho, ICMP_ECHO );
	ICMPH_CODE_SET( iecho, 0 );
	iecho->chksum = 0;
	iecho->id = lwip_htons( PingID );
	iecho->seqno = lwip_htons( ++seqNum );

	/* fill the additional data buffer with some data */
	for( int i = 0; i < PingLoadSize; i++ )
		( ( char* )iecho )[sizeof( icmp_echo_hdr ) + i] = ( char )i + 'a';

	iecho->chksum = inet_chksum( iecho, pingPacketSize );

	netbuf buf = {};
	err_t err;
	if( ( err = netbuf_ref( &buf, data, ( u16_t )pingPacketSize ) ) != ERR_OK )
		return err;
	ip_addr_t addr = IPADDR4_INIT_BYTES( address.addr[3], address.addr[2], address.addr[1], address.addr[0] );
	ip_addr_set( &buf.addr, &addr );
	buf.port = 0;
	err = netconn_send( conn, &buf );
	netbuf_free( &buf );

	return err;
}

void PingService::netconnCallback( netconn* conn, netconn_evt evt, u16_t len )
{
	chSysLock();
	if( evt == NETCONN_EVT_RCVPLUS )
	{
		chEvtSignalI( inst->thread_ref, PingService::NetconnRecvEvent );
		++inst->recvCount;
	}
	else if( evt == NETCONN_EVT_RCVMINUS )
		--inst->recvCount;
	else if( evt == NETCONN_EVT_ERROR )
		chEvtSignalI( inst->thread_ref, PingService::NetconnErrorEvent );
	chSchRescheduleS();
	chSysUnlock();
}

void PingService::timerCallback( void* p )
{
	chSysLockFromISR();
	chEvtSignalI( inst->thread_ref, PingService::TimerEvent );
	chVTSetI( &inst->timer, TIME_MS2I( 100 ), timerCallback, nullptr );
	chSysUnlockFromISR();
}