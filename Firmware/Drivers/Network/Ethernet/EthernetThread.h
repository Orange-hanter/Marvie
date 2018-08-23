#pragma once

#include "Core/BaseDynamicThread.h"
#include "Network/IpAddress.h"
#include <lwip/netifapi.h>
#include "hal.h"

class EthernetThread : public BaseDynamicThread
{
	EthernetThread();
	~EthernetThread();

public:
	enum Event : eventflags_t { StateChanged = 1, LinkStatusChanged = 2, NetworkAddressChanged = 4 };
	enum class AddressMode { Static, Dhcp, Auto };
	enum class LinkStatus { Up, Down };

	struct Config
	{
		uint8_t macAddress[6];
		IpAddress ipAddress;
		uint32_t netmask;
		IpAddress gateway;
		AddressMode addressMode;
		const char* hostName;
	};

	static EthernetThread* instance();

	void setConfig( const Config& config );
	Config currentConfig();
	bool startThread( tprio_t prio = NORMALPRIO );
	void stopThread();
	void waitForStop();

	LinkStatus linkStatus();
	IpAddress networkAddress();
	void setAsDefault();

	EvtSource* eventSource();

private:
	void main() override;

	static void lowLevelInit( struct netif *netif );
	static err_t lowLevelOutput( struct netif *netif, struct pbuf *p );
	static bool lowLevelInput( struct netif *netif, struct pbuf **pbuf );
	static err_t ethernetifInit( struct netif *netif );
	static void statusCallback( netif* nif );

private:
	enum
	{
		EthMacAddr0 = 0xC2,
		EthMacAddr1 = 0xAF,
		EthMacAddr2 = 0x51,
		EthMacAddr3 = 0x03,
		EthMacAddr4 = 0xCF,
		EthMacAddr5 = 0x46,
		EthNetIfName0 = 'e',
		EthNetIfName1 = 't',
		StopRequestEvent = 1,
		TimerEvent = 2,
		FrameReceivedEvent = 4,
		EthMtu = 1500,
		LinkPoolInterval = 2000,
		SendTimeout = 50		
	};
	static EthernetThread* ethThread;

	Config config;
	volatile bool started;
	MACConfig macConfig;
	netif thisif;
	EvtSource eSource;
};