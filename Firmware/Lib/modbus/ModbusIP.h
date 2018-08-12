#pragma once

#include "ModbusInterface.h"

namespace ModbusPotato
{
	/// <summary>
	/// This class handles the TCP/IP based protocol for Modbus.
	/// </summary>
	class CModbusIP : public IFramer
	{
	public:
		CModbusIP( IStream* stream, ITimeProvider* timer, uint8_t* buffer, size_t buffer_max );

		void set_master( bool master );
		bool master();
		unsigned long poll();

		bool begin_send();
		void send();
		void finished();

		bool frame_ready() const override { return m_state == state_frame_ready; }

	private:
		enum
		{
			header_size = 7,
			transaction_id_high_pos = 0,
			transaction_id_low_pos = 1,
			proto_id_high_pos = 2,
			proto_id_low_pos = 3,
			lenght_high_pos = 4,
			lenght_low_pos = 5,
			unit_id_pos = 6,
			unit_id_offset = 6,
			min_packet_length = 9,
		};
		volatile bool m_master;
		uint16_t transaction_id;
		enum state_type
		{
			state_exception,
			state_idle,
			state_frame_ready,
			state_queue,
			state_collision,
			state_tx_frame,
			state_tx_wait,
		};
		state_type m_state;
	};
}