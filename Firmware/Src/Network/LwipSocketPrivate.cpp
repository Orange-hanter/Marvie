#include "LwipSocketPrivate.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include <string.h>

LwipSocketPrivate::LwipSocketPrivate( netconn_type type )
{
	lastNetbuf = nullptr;
	offset = 0;
	recvCounter = 0;

	con = netconn_new_with_callback( type, netconnCallback );
	con->socket = ( int )this;

	prevState = state();
}

LwipSocketPrivate::LwipSocketPrivate( uint32_t )
{
	lastNetbuf = nullptr;
	offset = 0;
	recvCounter = 0;
	con = nullptr;
}

LwipSocketPrivate::~LwipSocketPrivate()
{
	if( con )
		netconn_delete( con );
	if( lastNetbuf )
		netbuf_delete( lastNetbuf );
}

void LwipSocketPrivate::setNetconn( netconn* con )
{
	this->con = con;
	con->callback = netconnCallback;
	con->socket = ( int )this;

	prevState = state();
}

void LwipSocketPrivate::recreate()
{
	netconn_type type = con->type;
	netconn_delete( con );
	con = netconn_new_with_callback( type, netconnCallback );
	con->socket = ( int )this;
	if( lastNetbuf )
		netbuf_delete( lastNetbuf ), lastNetbuf = nullptr;
	offset = 0;
	recvCounter = 0;
}

bool LwipSocketPrivate::bind( uint16_t port )
{
	if( !con->pcb.ip || ( con->type == NETCONN_TCP && con->pcb.tcp->state != tcp_state::CLOSED ) )
		recreate();
	err_t err = netconn_bind( con, IP_ADDR_ANY, port );
	updateState();

	return err == ERR_OK;
}

bool LwipSocketPrivate::bind( IpAddress address, uint16_t port )
{
	if( !con->pcb.ip || ( con->type == NETCONN_TCP && con->pcb.tcp->state != tcp_state::CLOSED ) )
		recreate();
	ip_addr_t addr = IPADDR4_INIT_BYTES( address.addr[3], address.addr[2], address.addr[1], address.addr[0] );
	err_t err = netconn_bind( con, &addr, port );
	updateState();

	return err == ERR_OK;
}

bool LwipSocketPrivate::connect( const char* hostName, uint16_t port )
{
	if( !con->pcb.ip || ( con->type == NETCONN_TCP && con->pcb.tcp->state != tcp_state::CLOSED ) )
		recreate();
	return false;
}

bool LwipSocketPrivate::connect( IpAddress address, uint16_t port )
{
	if( !con->pcb.ip || ( con->type == NETCONN_TCP && con->pcb.tcp->state != tcp_state::CLOSED ) )
		recreate();
	ip_addr_t addr = IPADDR4_INIT_BYTES( address.addr[3], address.addr[2], address.addr[1], address.addr[0] );
	err_t err = netconn_connect( con, &addr, port );
	updateState();

	return err == ERR_OK;
}

bool LwipSocketPrivate::isOpen() const
{
	if( con->type == NETCONN_UDP )
		return con->pcb.udp && con->state == NETCONN_NONE;
	else /*if( con->type == NETCONN_TCP )*/
		return con->pcb.tcp && con->pcb.tcp->state == tcp_state::ESTABLISHED;
}

SocketState LwipSocketPrivate::state()
{
	if( con->type == NETCONN_UDP )
	{
		if( !con->pcb.udp )
			return SocketState::Unconnected;
		if( con->pcb.udp->remote_port != 0 )
			return SocketState::Connected;
		if( con->pcb.udp->local_port != 0 )
			return SocketState::Bound;
		return SocketState::Unconnected;
	}
	else /*if( con->type == NETCONN_TCP )*/
	{
		if( !con->pcb.tcp )
			return SocketState::Unconnected;
		if( con->pcb.tcp->state == tcp_state::ESTABLISHED )
			return SocketState::Connected;
		if( con->pcb.tcp->state == tcp_state::SYN_SENT )
			return SocketState::Connecting;
		if( con->pcb.tcp->state == tcp_state::CLOSE_WAIT || con->pcb.tcp->state == tcp_state::LAST_ACK )
			return SocketState::Closing;
		return SocketState::Unconnected;
	}
}

SocketError LwipSocketPrivate::socketError() const
{
	return socketError( con );
}

SocketError LwipSocketPrivate::socketError( const netconn* con )
{
	switch( con->pending_err )
	{
	case err_enum_t::ERR_OK:
		if( con->recvmbox != 0 && con->type == NETCONN_TCP && con->pcb.tcp->state == tcp_state::CLOSE_WAIT )
			return SocketError::RemoteHostClosedError;
		else
			return SocketError::NoError;
	case err_enum_t::ERR_MEM:
		return SocketError::SocketResourceError;
	case err_enum_t::ERR_BUF:
		return SocketError::SocketResourceError;
	case err_enum_t::ERR_TIMEOUT:
		return SocketError::SocketTimeoutError;
	case err_enum_t::ERR_RTE:
		return SocketError::NetworkError;
	case err_enum_t::ERR_INPROGRESS:
		return SocketError::UnknownSocketError;
	case err_enum_t::ERR_VAL:
		return SocketError::IllegalArgument;
	case err_enum_t::ERR_WOULDBLOCK:
		return SocketError::UnknownSocketError;
	case err_enum_t::ERR_USE:
		return SocketError::AddressInUseError;
	case err_enum_t::ERR_ALREADY:
		return SocketError::UnknownSocketError;
	case err_enum_t::ERR_ISCONN:
		return SocketError::UnknownSocketError;
	case err_enum_t::ERR_CONN:
		return SocketError::UnknownSocketError;
	case err_enum_t::ERR_IF:
		return SocketError::NetworkError;

	case err_enum_t::ERR_ABRT:
		return SocketError::SocketResourceError;
	case err_enum_t::ERR_RST:
		return SocketError::ConnectionRefusedError;
	case err_enum_t::ERR_CLSD:
		return SocketError::RemoteHostClosedError;
	case err_enum_t::ERR_ARG:
		return SocketError::IllegalArgument;

	default:
		return SocketError::UnknownSocketError;
	}
}

uint32_t LwipSocketPrivate::read( uint8_t* data, uint32_t size, sysinterval_t timeout )
{
	if( size == 0 )
		return 0;
	if( timeout == TIME_IMMEDIATE )
	{
		uint32_t n = readAvailable();
		if( n == 0 )
			return 0;
		if( n < size )
			size = n;
	}

	uint32_t max = size;
	systime_t deadline = chVTGetSystemTimeX() + ( systime_t )timeout;
	sysinterval_t nextTimeout = timeout;
	while( true )
	{
		chSysLock();
		if( readAvailable() == 0 )
		{
			msg_t msg = MSG_RESET;
			if( isOpen() )
				msg = readWaitingQueue.enqueueSelf( nextTimeout );
			chSysUnlock();
			if( msg != MSG_OK )
				return max - size;
		}
		else
			chSysUnlock();
		if( !lastNetbuf )
			takeNetbuf();

		while( true )
		{
			uint8_t* payload;
			u16_t len;
			netbuf_data( lastNetbuf, ( void** )&payload, &len );

			uint32_t part = len - offset;
			if( part > size )
				part = size;
			if( data )
			{
				memcpy( data, payload + offset, part );
				data += part;
			}
			offset += part;
			size -= part;
			if( offset == len )
			{
				offset = 0;
				if( netbuf_next( lastNetbuf ) < 0 )
				{
					netbuf_delete( lastNetbuf );
					lastNetbuf = nullptr;

					if( size == 0 )
						return max;
					break;
				}
				else if( con->type == NETCONN_TCP )
					deleteFirstPBuf();
			}

			if( size == 0 )
				return max;
		}

		if( timeout != TIME_INFINITE )
		{
			sysinterval_t nextTimeout = ( sysinterval_t )( deadline - chVTGetSystemTimeX() );
			if( nextTimeout > timeout ) // timeout expired
				return max - size;
		}
	}
}

uint32_t LwipSocketPrivate::netbufPeek( uint32_t pos, uint8_t* data, uint32_t size )
{
	pbuf* p = lastNetbuf->ptr;
	uint32_t offset = this->offset;
	uint32_t max = size;

	while( pos )
	{
		uint32_t part = p->len - offset;
		if( part > pos )
			part = pos;
		offset += part;
		if( offset == p->len )
		{
			p = p->next;
			if( !p )
				return 0;
			offset = 0;
		}
		pos -= part;
	}

	while( size )
	{
		uint32_t part = p->len - offset;
		if( part > size )
			part = size;
		memcpy( data, ( uint8_t* )p->payload + offset, part );
		data += part;
		offset += part;
		size -= part;
		if( offset == p->len )
		{
			p = p->next;
			if( !p )
				return max - size;
			offset = 0;
		}
	}

	return max;
}

uint32_t LwipSocketPrivate::readAvailable() const
{
	uint32_t n = ( uint32_t )( recvCounter < 0 ? 0 : recvCounter );
	if( lastNetbuf )
		n += lastNetbuf->p->tot_len - offset;
	return n;
}

bool LwipSocketPrivate::waitForReadAvailable( uint32_t size, sysinterval_t timeout )
{
	systime_t deadline = chVTGetSystemTimeX() + ( systime_t )timeout;

	chSysLock();
	while( size > readAvailable() )
	{
		if( !isOpen() )
		{
			chSysUnlock();
			return false;
		}

		msg_t msg;
		if( timeout == TIME_INFINITE || timeout == TIME_IMMEDIATE )
			msg = readWaitingQueue.enqueueSelf( timeout );
		else
		{
			sysinterval_t nextTimeout = ( sysinterval_t )( deadline - chVTGetSystemTimeX() );

			// timeout expired
			if( nextTimeout > timeout )
				break;

			msg = readWaitingQueue.enqueueSelf( nextTimeout );
		}

		// timeout expired or queue reseted
		if( msg != MSG_OK )
			break;
	}

	chSysUnlock();
	return size <= readAvailable();
}

void LwipSocketPrivate::setSocketOption( SocketOption option, int value )
{
	if( con->type != NETCONN_TCP )
		return;
	switch( option )
	{
	case SocketOption::LowDelay:
		if( value )
			tcp_nagle_disable( con->pcb.tcp );
		else
			tcp_nagle_enable( con->pcb.tcp );
		break;
	case SocketOption::KeepAlive:
		con->pcb.tcp->keep_idle = ( u32_t )( value );
		break;
	default:
		break;
	}
}

int LwipSocketPrivate::socketOption( SocketOption option )
{
	if( con->type != NETCONN_TCP )
		return -1;
	switch( option )
	{
	case SocketOption::LowDelay:
		return tcp_nagle_disabled( con->pcb.tcp );
	case SocketOption::KeepAlive:
		return ( int )con->pcb.tcp->keep_idle;
	default:
		return -1;
	}
}

uint32_t LwipSocketPrivate::takeNetbuf()
{
	if( netconn_recv( con, &lastNetbuf ) == ERR_OK )
	{
		chSysLock();
		recvCounter -= lastNetbuf->p->tot_len;
		chSysUnlock();

		return lastNetbuf->p->tot_len;
	}

	return 0;
}

void LwipSocketPrivate::deleteFirstPBuf()
{
	pbuf* first = lastNetbuf->p;
	lastNetbuf->p = first->next;
	first->next = nullptr;
	pbuf_free( first );
}

void LwipSocketPrivate::catNetbuf()
{
	netbuf* head = lastNetbuf;
	takeNetbuf();
	pbuf_cat( head->p, lastNetbuf->p );
	lastNetbuf->p = lastNetbuf->ptr = nullptr;
	netbuf_delete( lastNetbuf );
	lastNetbuf = head;
}

void LwipSocketPrivate::updateState()
{
	chSysLock();
	updateStateS();
	chSchRescheduleS();
	chSysUnlock();
}

void LwipSocketPrivate::updateStateS()
{
	auto current = state();
	if( prevState != current )
	{
		evtSource.broadcastFlags( ( eventflags_t )SocketEventFlag::StateChanged );
		prevState = current;
	}
}

void LwipSocketPrivate::netconnCallback( netconn* con, netconn_evt evt, u16_t len )
{
	LwipSocketPrivate* socket = ( ( LwipSocketPrivate* )con->socket );
	if( evt == NETCONN_EVT_RCVPLUS )
	{
		chSysLock();
		socket->recvCounter += len;
		if( len )
			socket->readWaitingQueue.dequeueNext( MSG_OK );
		else
		{
			socket->updateStateS();
			socket->readWaitingQueue.dequeueNext( MSG_RESET );
		}
		socket->evtSource.broadcastFlags( ( eventflags_t )SocketEventFlag::InputAvailable );
		chSchRescheduleS();
		chSysUnlock();
	}
	else if( evt == NETCONN_EVT_ERROR )
	{
		chSysLock();
		socket->readWaitingQueue.dequeueNext( MSG_RESET );
		socket->evtSource.broadcastFlags( ( eventflags_t )SocketEventFlag::Error );
		socket->updateStateS();
		chSchRescheduleS();
		chSysUnlock();
	}
}