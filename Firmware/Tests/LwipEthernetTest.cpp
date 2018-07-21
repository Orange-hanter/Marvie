#include "hal.h"
#include "lwipthread.h"
#include "Lib/lwip/src/include/lwip/api.h"
#include "Core/Concurrent.h"
#include "Core/ObjectMemoryUtilizer.h"

int main()
{
	halInit();
	chSysInit();

	ObjectMemoryUtilizer::instance()->runUtilizer( NORMALPRIO );

	lwipInit( NULL );
	
	auto conn = netconn_new( NETCONN_TCP );
	netconn* newconn;
	if( conn != NULL )
	{
		/* Bind connection to well known port number 7. */
		auto err = netconn_bind( conn, NULL, 7 );
		if( err == ERR_OK )
		{
			/* Tell connection to go into listening mode. */
			netconn_listen( conn );
			while( 1 )
			{
				/* Grab new connection. */
				auto accept_err = netconn_accept( conn, &newconn );
				/* Process the new connection. */
				if( accept_err == ERR_OK )
				{
					Concurrent::run( [newconn]()
					{
						err_t recv_err;
						netbuf* buf;
						while( ( recv_err = netconn_recv( newconn, &buf ) ) == ERR_OK )
						{
							do
							{
								void* data;
								u16_t len;
								netbuf_data( buf, &data, &len );
								netconn_write( newconn, data, len, NETCONN_COPY );
							} while( netbuf_next( buf ) >= 0 );

							netbuf_delete( buf );
						}
						/* Close connection and discard connection identifier. */
						netconn_close( newconn );
						netconn_delete( newconn );
					}, 2048, NORMALPRIO );
				}
			}
		}
		else
		{
			netconn_delete( newconn );
		}
	}

	thread_reference_t ref = nullptr;
	chSysLock();
	chThdSuspendS( &ref );

	return 0;
}