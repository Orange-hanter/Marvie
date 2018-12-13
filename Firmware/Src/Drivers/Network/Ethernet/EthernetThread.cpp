#include "EthernetThread.h"

#include "evtimer.h"
#include <string.h>

#include <lwip/opt.h>
#include <lwip/def.h>
#include <lwip/mem.h>
#include <lwip/pbuf.h>
#include <lwip/sys.h>
#include <lwip/stats.h>
#include <lwip/snmp.h>
#include <lwip/tcpip.h>
#include <netif/etharp.h>

#if LWIP_DHCP
#include <lwip/dhcp.h>
#endif

#if LWIP_AUTOIP
#include <lwip/autoip.h>
#endif

EthernetThread* EthernetThread::ethThread = nullptr;

EthernetThread::EthernetThread() : BaseDynamicThread( 1024 )
{
	config.macAddress[0] = EthMacAddr0;
	config.macAddress[1] = EthMacAddr1;
	config.macAddress[2] = EthMacAddr2;
	config.macAddress[3] = EthMacAddr3;
	config.macAddress[4] = EthMacAddr4;
	config.macAddress[5] = EthMacAddr5;
	config.ipAddress = IpAddress( 192, 168, 1, 10 );
	config.netmask = 0xFFFFFF00;
	config.gateway = IpAddress( 192, 168, 1, 1 );
	config.addressMode = AddressMode::Dhcp;
	config.hostName = "lwip";
	started = false;
	macConfig.mac_address = config.macAddress;
	memset( &thisif, 0, sizeof( netif ) );
}

EthernetThread::~EthernetThread()
{
	stopThread();
	waitForStop();
}

EthernetThread* EthernetThread::instance()
{
	if( ethThread )
		return ethThread;
	ethThread = new EthernetThread;
	return ethThread;
}

void EthernetThread::setConfig( const Config& config )
{
	if( started )
		return;
	this->config = config;
}

EthernetThread::Config EthernetThread::currentConfig()
{
	return config;
}

bool EthernetThread::startThread( tprio_t prio /*= NORMALPRIO */ )
{
	if( started )
		return false;
	started = true;
	eSource.broadcastFlags( StateChanged );
	start( prio );
	return true;
}

void EthernetThread::stopThread()
{
	if( !started )
		return;
	chEvtSignal( thread_ref, StopRequestEvent );
}

void EthernetThread::waitForStop()
{
	if( started )
		wait();
}

EthernetThread::LinkStatus EthernetThread::linkStatus()
{
	return netif_is_link_up( &thisif ) ? LinkStatus::Up : LinkStatus::Down;
}

IpAddress EthernetThread::networkAddress()
{
	syssts_t sysStatus = chSysGetStatusAndLockX();
	if( linkStatus() == LinkStatus::Down )
	{
		chSysRestoreStatusX( sysStatus );
		return IpAddress();
	}
	uint32_t addr = PP_HTONL( thisif.ip_addr.addr );
	chSysRestoreStatusX( sysStatus );
	return IpAddress( addr );
}

uint32_t EthernetThread::networkMask()
{
	return PP_HTONL( thisif.netmask.addr );
}

IpAddress EthernetThread::networkGateway()
{
	uint32_t addr = PP_HTONL( thisif.gw.addr );
	return IpAddress( addr );
}

void EthernetThread::setAsDefault()
{
	netif_set_default( &thisif );
}

EvtSource* EthernetThread::eventSource()
{
	return &eSource;
}

void EthernetThread::main()
{
	event_timer_t evt;
	event_listener_t el0, el1;
	ip_addr_t ip, gateway, netmask;

	chRegSetThreadName( "EthernetThread" );
	thisif.hwaddr[0] = config.macAddress[0];
	thisif.hwaddr[1] = config.macAddress[1];
	thisif.hwaddr[2] = config.macAddress[2];
	thisif.hwaddr[3] = config.macAddress[3];
	thisif.hwaddr[4] = config.macAddress[4];
	thisif.hwaddr[5] = config.macAddress[5];
	ip.addr = PP_HTONL( *( uint32_t* )&config.ipAddress );
	gateway.addr = PP_HTONL( *( uint32_t* )&config.gateway );
	netmask.addr = PP_HTONL( config.netmask );
#if LWIP_NETIF_HOSTNAME
	thisif.hostname = config.hostName;
#endif

	macConfig = { thisif.hwaddr };
	macStart( &ETHD1, &macConfig );

	/* Add interface. */
	LOCK_TCPIP_CORE();
	if( !netif_add( &thisif, &ip, &netmask, &gateway, nullptr, ethernetifInit, tcpip_input ) )
		goto End;
	netif_set_up( &thisif );
	netif_set_link_down( &thisif );
	netif_get_client_data( &thisif, LWIP_NETIF_CLIENT_DATA_INDEX_MAX ) = this;
	netif_set_status_callback( &thisif, statusCallback );
#if LWIP_AUTOIP
	if( config.addressMode == AddressMode::Auto )
		autoip_start( &thisif );
#endif
	UNLOCK_TCPIP_CORE();

	/* Setup event sources.*/
	evtObjectInit( &evt, TIME_MS2I( LinkPoolInterval ) );
	evtStart( &evt );
	chEvtRegisterMask( &evt.et_es, &el0, TimerEvent );
	chEvtRegisterMask( macGetReceiveEventSource( &ETHD1 ), &el1, FrameReceivedEvent );
	chEvtAddEvents( TimerEvent | FrameReceivedEvent );

	while( true )
	{
		eventmask_t mask = chEvtWaitAny( ALL_EVENTS );
		if( mask & StopRequestEvent )
			break;
		if( mask & TimerEvent )
		{
			bool current_link_status = macPollLinkStatus( &ETHD1 );
			if( current_link_status != netif_is_link_up( &thisif ) )
			{
				if( current_link_status )
				{
					LOCK_TCPIP_CORE();
					netif_set_link_up( &thisif );
#if LWIP_DHCP
					if( config.addressMode == AddressMode::Dhcp )
						dhcp_start( &thisif );
#endif
					UNLOCK_TCPIP_CORE();
				}
				else
				{
					LOCK_TCPIP_CORE();
					netif_set_link_down( &thisif );
#if LWIP_DHCP
					if( config.addressMode == AddressMode::Dhcp )
						dhcp_stop( &thisif );
#endif
					UNLOCK_TCPIP_CORE();
				}
				eSource.broadcastFlags( LinkStatusChanged );
			}
		}
		if( mask & FrameReceivedEvent )
		{
			pbuf *p;
			while( lowLevelInput( &thisif, &p ) )
			{
				if( p != NULL )
				{
					eth_hdr *ethhdr = ( eth_hdr* )p->payload;
					switch( htons( ethhdr->type ) )
					{
						/* IP or ARP packet? */
					case ETHTYPE_IP:
					case ETHTYPE_ARP:
						/* full packet send to tcpip_thread to process */
						if( thisif.input( p, &thisif ) == ERR_OK )
							break;
						LWIP_DEBUGF( NETIF_DEBUG, ( "ethernetif_input: IP input error\n" ) );
						/* Falls through */
					default:
						pbuf_free( p );
					}
				}
			}
		}
	}

	evtStop( &evt );
	chEvtUnregister( &evt.et_es, &el0 );
	chEvtUnregister( macGetReceiveEventSource( &ETHD1 ), &el1 );

	LOCK_TCPIP_CORE();
	if( netif_is_link_up( &thisif ) )
		eSource.broadcastFlags( LinkStatusChanged );
End:
	netif_set_down( &thisif );
#if LWIP_DHCP
	if( config.addressMode == AddressMode::Dhcp )
		dhcp_stop( &thisif );
#endif
#if LWIP_AUTOIP
	if( config.addressMode == AddressMode::Auto )
		autoip_stop( &thisif );
#endif
	netif_remove( &thisif );
	UNLOCK_TCPIP_CORE();

	macStop( &ETHD1 );

	chSysLock();
	eSource.broadcastFlagsI( StateChanged );
	started = false;
	exitS( MSG_OK );
}

void EthernetThread::lowLevelInit( netif *netif )
{
	/* set MAC hardware address length */
	netif->hwaddr_len = ETHARP_HWADDR_LEN;

	/* maximum transfer unit */
	netif->mtu = EthMtu;

	/* device capabilities */
	/* don't set NETIF_FLAG_ETHARP if this device is not an Ethernet one */
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

	/* Do whatever else is needed to initialize interface. */
}

/*
* This function does the actual transmission of the packet. The packet is
* contained in the pbuf that is passed to the function. This pbuf
* might be chained.
*
* @param netif the lwip network interface structure for this ethernetif
* @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
* @return ERR_OK if the packet could be sent
*         an err_t value if the packet couldn't be sent
*
* @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
*       strange results. You might consider waiting for space in the DMA queue
*       to become available since the stack doesn't retry to send a packet
*       dropped because of memory failure (except for the TCP timers).
*/
err_t EthernetThread::lowLevelOutput( netif *netif, pbuf *p )
{
	pbuf *q;
	MACTransmitDescriptor td;

	if( macWaitTransmitDescriptor( &ETHD1, &td, TIME_MS2I( SendTimeout ) ) != MSG_OK )
		return ERR_TIMEOUT;

#if ETH_PAD_SIZE
	pbuf_header( p, -ETH_PAD_SIZE ); /* drop the padding word */
#endif

	/* Iterates through the pbuf chain. */
	for( q = p; q != NULL; q = q->next )
		macWriteTransmitDescriptor( &td, ( uint8_t * )q->payload, ( size_t )q->len );
	macReleaseTransmitDescriptor( &td );

	MIB2_STATS_NETIF_ADD( netif, ifoutoctets, p->tot_len );
	if( ( ( u8_t* )p->payload )[0] & 1 )
		MIB2_STATS_NETIF_INC( netif, ifoutnucastpkts ); /* broadcast or multicast packet*/
	else
		MIB2_STATS_NETIF_INC( netif, ifoutucastpkts ); /* unicast packet */
	/* increase ifoutdiscards or ifouterrors on error */

#if ETH_PAD_SIZE
	pbuf_header( p, ETH_PAD_SIZE ); /* reclaim the padding word */
#endif

	LINK_STATS_INC( link.xmit );

	return ERR_OK;
}

/*
* Receives a frame.
* Allocates a pbuf and transfers the bytes of the incoming
* packet from the interface into the pbuf.
*
* @param netif the lwip network interface structure for this ethernetif
* @return a pbuf filled with the received packet (including MAC header)
*         NULL on memory error
*/
bool EthernetThread::lowLevelInput( netif *netif, pbuf **pbuf )
{
	MACReceiveDescriptor rd;
	struct pbuf *q;
	u16_t len;

	osalDbgAssert( pbuf != NULL, "invalid null pointer" );

	if( macWaitReceiveDescriptor( &ETHD1, &rd, TIME_IMMEDIATE ) != MSG_OK )
		return false;

	len = ( u16_t )rd.size;

#if ETH_PAD_SIZE
	len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

	/* We allocate a pbuf chain of pbufs from the pool. */
	*pbuf = pbuf_alloc( PBUF_RAW, len, PBUF_POOL );

	if( *pbuf != NULL )
	{
#if ETH_PAD_SIZE
		pbuf_header( pbuf, -ETH_PAD_SIZE ); /* drop the padding word */
#endif

		/* Iterates through the pbuf chain. */
		for( q = *pbuf; q != NULL; q = q->next )
			macReadReceiveDescriptor( &rd, ( uint8_t * )q->payload, ( size_t )q->len );
		macReleaseReceiveDescriptor( &rd );

		MIB2_STATS_NETIF_ADD( netif, ifinoctets, *pbuf->tot_len );

		if( *( uint8_t * )( ( *pbuf )->payload ) & 1 )
			MIB2_STATS_NETIF_INC( netif, ifinnucastpkts ); /* broadcast or multicast packet*/
		else
			MIB2_STATS_NETIF_INC( netif, ifinucastpkts ); /* unicast packet*/

#if ETH_PAD_SIZE
		pbuf_header( pbuf, ETH_PAD_SIZE ); /* reclaim the padding word */
#endif
		LINK_STATS_INC( link.recv );
	}
	else
	{
		macReleaseReceiveDescriptor( &rd ); // Drop packet
		LINK_STATS_INC( link.memerr );
		LINK_STATS_INC( link.drop );
		MIB2_STATS_NETIF_INC( netif, ifindiscards );
	}

	return true;
}

/*
* Called at the beginning of the program to set up the
* network interface. It calls the function low_level_init() to do the
* actual setup of the hardware.
*
* This function should be passed as a parameter to netifapi_netif_add().
*
* @param netif the lwip network interface structure for this ethernetif
* @return ERR_OK if the loopif is initialised
*         ERR_MEM if private data couldn't be allocated
*         any other err_t on error
*/
err_t EthernetThread::ethernetifInit( netif *netif )
{
	osalDbgAssert( ( netif != NULL ), "netif != NULL" );

	/*
	* Initialize the snmp variables and counters inside the struct netif.
	* The last argument should be replaced with your link speed, in units
	* of bits per second.
	*/
	MIB2_INIT_NETIF( netif, snmp_ifType_ethernet_csmacd, LWIP_LINK_SPEED );

	netif->state = NULL;
	netif->name[0] = EthNetIfName0;
	netif->name[1] = EthNetIfName1;
	/* We directly use etharp_output() here to save a function call.
	* You can instead declare your own function an call etharp_output()
	* from it if you have to do some checks before sending (e.g. if link
	* is available...) */
	netif->output = etharp_output;
	netif->linkoutput = lowLevelOutput;

	/* initialize the hardware */
	lowLevelInit( netif );

	return ERR_OK;
}

void EthernetThread::statusCallback( netif* nif )
{
	reinterpret_cast< EthernetThread* >( netif_get_client_data( nif, LWIP_NETIF_CLIENT_DATA_INDEX_MAX ) )->eSource.broadcastFlags( NetworkAddressChanged );
}