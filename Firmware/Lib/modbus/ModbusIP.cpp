#include "ModbusIP.h"

#ifdef _MSC_VER
#undef max
#endif
#define ELAPSED(start, end) ((system_tick_t)(end) - (system_tick_t)(start))
#ifdef _MSC_VER
static_assert( ~( system_tick_t )0 > 0, "system_tick_t must be unsigned" );
static_assert( ( system_tick_t )-1 == ~( system_tick_t )0, "two's complement arithmetic required" );
static_assert( ELAPSED( ~( system_tick_t )0, 0 ) == 1, "elapsed time roll-over check failed" );
#endif

using namespace ModbusPotato;

CModbusIP::CModbusIP( IStream* stream, ITimeProvider* timer, uint8_t* buffer, size_t buffer_max )
	: IFramer( stream, timer, buffer, buffer_max )
	, m_master( false )
	, transaction_id( 0 )
	, m_state( state_idle )
{
	if( !m_stream || !m_timer || !m_buffer || m_buffer_max < 9 )
	{
		m_state = state_exception;
		return;
	}
}

void ModbusPotato::CModbusIP::set_master( bool master )
{
	m_master = master;
}

bool ModbusPotato::CModbusIP::master()
{
	return m_master;
}

unsigned long ModbusPotato::CModbusIP::poll()
{
	switch( m_state )
	{
	case ModbusPotato::CModbusIP::state_idle:
idle:
	{
		if( int ec = m_stream->read( m_buffer, header_size ) )
		{
			m_stream->communicationStatus( true, false );
			if( ec < header_size || m_buffer[proto_id_high_pos] != 0 || m_buffer[proto_id_low_pos] != 0 )
			{
				m_stream->read( nullptr, ( size_t )-1 );
				m_stream->communicationStatus( false, false );
				return 0;
			}

			uint16_t frame_transaction_id = m_buffer[transaction_id_high_pos];
			frame_transaction_id <<= 8;
			frame_transaction_id |= m_buffer[transaction_id_low_pos];
			uint16_t frame_lenght = m_buffer[lenght_high_pos];
			frame_lenght <<= 8;
			frame_lenght |= m_buffer[lenght_low_pos];
			m_frame_address = m_buffer[unit_id_pos];
			m_buffer_len = frame_lenght - 1;
			if( m_buffer_len > m_buffer_max )
			{
				m_stream->read( nullptr, ( size_t )-1 );
				m_stream->communicationStatus( false, false );
				return 0;
			}

			ec = m_stream->read( m_buffer, frame_lenght - 1 ) + 1;
			m_stream->communicationStatus( false, false );

			if( frame_lenght > ec || ( m_master && frame_transaction_id != transaction_id ) )
			{
				m_stream->read( nullptr, ( size_t )-1 );
				return 0;
			}

			m_buffer_len = frame_lenght - 1;
			transaction_id = frame_transaction_id;
			m_state = state_frame_ready;
			if( m_handler )
				m_handler->frame_ready( this );

			return poll();
		}

		return 0;
	}
	case ModbusPotato::CModbusIP::state_frame_ready:
	case ModbusPotato::CModbusIP::state_queue:
	{
		if( m_stream->read( nullptr, ( size_t )-1 ) )
		{
			m_state = state_collision;
			m_stream->communicationStatus( false, false );
		}
		return 0;
	}
	case ModbusPotato::CModbusIP::state_collision:
	{
		return 0;
	}
	case ModbusPotato::CModbusIP::state_tx_frame:
	{
		if( m_stream->write( m_buffer, m_buffer_len ) != ( int )m_buffer_len )
		{
			m_state = state_exception;
			m_stream->communicationStatus( false, false );
			return 0; // fatal exception
		}
		m_state = state_tx_wait;
	}
	case ModbusPotato::CModbusIP::state_tx_wait:
	{
		// poll if the write has completed
		if( m_stream->writeComplete() )
		{
			// transmission complete
			m_stream->txEnable( false );

			// done! go to the idle state
			m_state = state_idle;
			m_stream->communicationStatus( false, false );
			goto idle;
		}

		return 0; // waiting for write buffer to drain
	}
	case ModbusPotato::CModbusIP::state_exception:
		return 0;
	default:
		m_state = state_exception;
		return 0;
	}
}

bool ModbusPotato::CModbusIP::begin_send()
{
	switch( m_state )
	{
	case state_collision:
		return true; // if there was a collision then return true so that the user will call send() or finished()
	case state_queue:
		return true; // already in the queue state
	case state_idle:
	case state_frame_ready:
	{
		m_state = state_queue; // set the state machine to the 'queue' state we the user can access the buffer
		return true;
	}
	default:
		return false; // not ready to send
	}
}

void ModbusPotato::CModbusIP::send()
{
	// sanity check
	if( m_buffer_len + header_size > buffer_max() )
	{
		// buffer overflow - enter the 'exception' state
		m_state = state_exception;
		return;
	}

	switch( m_state )
	{
	case state_queue: // buffer is ready
	{
		for( int i = m_buffer_len - 1; i >= 0; --i )
			m_buffer[i + header_size] = m_buffer[i];
		if( m_master )
			++transaction_id;
		m_buffer[0] = transaction_id >> 8;
		m_buffer[1] = transaction_id;
		m_buffer[2] = m_buffer[3] = 0;
		uint16_t len = m_buffer_len + 1;
		m_buffer[4] = len >> 8;
		m_buffer[5] = len;
		m_buffer[6] = m_frame_address;
		m_buffer_len += header_size;

		// enter the frame transmit
		m_state = state_tx_frame;
		m_stream->communicationStatus( false, true );

		// enable the transmitter
		m_stream->txEnable( true );
		return; // ok -- we expect that the user must call poll() at this point.
	}
	case state_collision:
	{
		// abort the response and go to the idle state
		m_state = state_idle;
		return; // collision, abort transmission and dump any further incoming data
	}
	default:
	{
		// invalid state, user probably didn't call begin_send()
		m_state = state_exception;
		return; // invalid state - enter the 'exception' state
	}
	}
}

void ModbusPotato::CModbusIP::finished()
{
	switch( m_state )
	{
	case state_frame_ready: // received
	case state_queue: // aborting begin_send()
	{
		// acknowledge or abort the user lock on the buffer
		m_state = state_idle;
		return; // ok
	}
	case state_collision: // bus collision
	{
		// more data started when we were not expecting it
		m_state = state_idle;
		return; // collision, dump any further incoming data
	}
	default:
	{
		// invalid state
		m_state = state_exception;
		return; // invalid state - enter the 'exception' state
	}
	}
}