#include "hal.h"
#include "Lib/modbus/ModbusRTU.h"
#include "Lib/modbus/ModbusASCII.h"
#include "Lib/modbus/ModbusSlave.h"

namespace ModbusTest
{
	using namespace ModbusPotato;

	static uint8_t m_frame_buffer[MODBUS_ASCII_DATA_BUFFER_SIZE];
#define MODBUS_REG_COUNT 512
	static uint16_t modbusRegisters[MODBUS_REG_COUNT];

	class ChibiTimerProvider : public ITimeProvider
	{
	public:
		ModbusPotato::system_tick_t ticks() const
		{
			return chVTGetSystemTimeX();
		}
		unsigned long microseconds_per_tick() const
		{
			return 1000000 / CH_CFG_ST_FREQUENCY;
		}
	} chibiTimeProvider;

	class SerialStream : public IStream
	{
	public:
		SerialStream( Usart* usart ) : usart( usart ) {}

		int read( uint8_t* buffer, size_t buffer_size )
		{
			if( !usart )
				return 0;

			uint32_t a = usart->readAvailable();
			if( a == 0 )
				return a;

			// make sure we don't over-run the end of the buffer
			if( buffer_size != ( size_t )-1 && a > buffer_size )
				a = buffer_size;

			// if there is no buffer provided, then dump the input and return the number of characters dumped
			if( !buffer )
			{
				usart->read( nullptr, a, TIME_INFINITE );
				return a;
			}

			// read the data
			return usart->read( buffer, a, TIME_INFINITE );
		}
		int write( uint8_t* buffer, size_t len )
		{
			if( !usart )
				return 0;

			// check how many characters can be written without blocking
			uint32_t a = usart->outputBuffer()->writeAvailable();
			if( a == 0 )
				return a; // nothing to do

			// limit the amount to write based on the available space
			if( len > a )
				len = a;

			// write the data
			return usart->write( buffer, len, TIME_INFINITE );
		}
		void txEnable( bool state ) {}
		bool writeComplete()
		{
			return usart->waitForBytesWritten( TIME_IMMEDIATE );
		}
		void communicationStatus( bool rx, bool tx ) {}

	private:
		Usart * usart;
	};

	class ModbusSlaveHandlerHolding : public ISlaveHandler
	{
	public:
		modbus_exception_code::modbus_exception_code read_holding_registers( uint16_t address, uint16_t count, uint16_t* result ) override
		{
			if( count > MODBUS_REG_COUNT || address >= MODBUS_REG_COUNT || ( size_t )( address + count ) > MODBUS_REG_COUNT )
				return modbus_exception_code::illegal_data_address;

			// copy the values
			for( ; count; address++, count-- )
				*result++ = modbusRegisters[address];

			return modbus_exception_code::ok;
		}
	} modbusSlaveHandlerHolding;

	void timerCallback( void* p )
	{
		chSysLockFromISR();
		chEvtSignalI( ( thread_t* )p, 2 );
		chSysUnlockFromISR();
	}

	int test()
	{
		/*struct Reg
		{
			int a;
			float b;
			uint8_t c[2];
			uint16_t d;
		};
		Reg* reg = ( Reg* )modbusRegisters;
		reg->a = 424204242;
		reg->b = 3.14;
		reg->c[0] = 1;
		reg->c[1] = 2;
		reg->d = 789;*/
		for( int i = 0; i < MODBUS_REG_COUNT; ++i )
			modbusRegisters[i] = i;

		palSetPadMode( GPIOA, 9, PAL_MODE_ALTERNATE( GPIO_AF_USART1 ) | PAL_STM32_OSPEED_HIGHEST );
		palSetPadMode( GPIOA, 10, PAL_MODE_ALTERNATE( GPIO_AF_USART1 ) | PAL_STM32_OSPEED_HIGHEST );
		Usart* usart = Usart::instance( &SD1 );
		static uint8_t ob[128];
		usart->setOutputBuffer( ob, 1 );
		usart->open();

		SerialStream* serialStream = new SerialStream( usart );
		CModbusSlave* modbusSlave = new CModbusSlave( &modbusSlaveHandlerHolding );
		/*CModbusRTU* modbus = new CModbusRTU( serialStream, &chibiTimeProvider, m_frame_buffer, sizeof( m_frame_buffer ) );
		modbus->setup( 115200 );
		modbus->set_handler( modbusSlave );
		modbus->set_station_address( 1 );*/
		CModbusASCII* modbus = new CModbusASCII( serialStream, &chibiTimeProvider, m_frame_buffer, sizeof( m_frame_buffer ) );
		modbus->set_handler( modbusSlave );
		modbus->set_station_address( 1 );

		EventListener listener;
		virtual_timer_t time;
		chVTObjectInit( &time );
		usart->eventSource().registerOne( &listener, 0 );

		while( true )
		{
			eventmask_t em = chEvtWaitAny( ALL_EVENTS );
			unsigned int nextInterval = modbus->poll();
			if( nextInterval )
				chVTSet( &time, ( sysinterval_t )nextInterval, timerCallback, chThdGetSelfX() );
			else
			{
				chVTReset( &time );
				chEvtGetAndClearEvents( 2 ); // timer event
			}
		}

		return 0;
	}
}