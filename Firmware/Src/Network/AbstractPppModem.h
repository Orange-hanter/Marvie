#pragma once

#include "Modem.h"
#include "lwip/api.h"
#include "lwip/tcpip.h"
extern "C"
{
#include "netif/ppp/pppos.h"
}

class AbstractPppModem : public Modem
{
public:
	AbstractPppModem( uint32_t stackSize );
	~AbstractPppModem();

	bool startModem( tprio_t prio = NORMALPRIO ) override;
	void setAsDefault();

protected:
	enum class LowLevelError { NoError, StopRequest, TimeoutError, SimCardNotInsertedError, PinCodeError, UnknownError };
	virtual LowLevelError lowLevelStart() = 0;
	virtual void lowLevelStop() = 0;

private:
	void main() final override;

	static u32_t outputCallback( ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx );
	static void linkStatusCallback( ppp_pcb *pcb, int err_code, void *ctx );
	static void netifStatusCallback( netif *nif );
	static void timerCallback( void* p );

private:
	enum { IOEvent = 2, TimerEvent = 4, LinkDownEvent = 8, LinkUpEvent = 16 };
	netif pppNetif;
	ppp_pcb* pppPcb;
	int attemptNumber;
	uint32_t len;
	uint8_t buffer[256];
	virtual_timer_t timer;
};