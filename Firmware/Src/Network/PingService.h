#include "lwip/api.h"
#include "Core/BaseDynamicThread.h"
#include "Core/NanoList.h"
#include "Network/IpAddress.h"

class PingService : public BaseDynamicThread
{
	PingService();
	~PingService();

public:
    enum class State { Stopped, Working, Stopping };
    enum class Error { NoError, MemoryError, NetworkError };
	enum class Event { Error = 1, StateChanged = 2 };

	static PingService* instance();

    bool startService( tprio_t prio = NORMALPRIO );
    void stopService();
    bool waitForStopped( sysinterval_t timeout = TIME_INFINITE );
    State state();
    Error error();

	struct TimeMeasurement
	{
		int32_t minMs;
		int32_t maxMs;
		int32_t avgMs;
	};
	int ping( IpAddress addr, uint32_t count = 1, TimeMeasurement* pTM = nullptr, uint32_t delayMs = 1000 );
	void setPongTimeout( uint32_t timeoutMs );

	EvtSource* eventSource();

private:
	void main() override;
	err_t sendPing( IpAddress address );
	static void netconnCallback( netconn* conn, netconn_evt evt, u16_t len );
	static void timerCallback( void* p );

private:
	static PingService* inst;
	enum : eventmask_t { StopRequestEvent = 1, PingRequestEvent = 2, NetconnRecvEvent = 4, NetconnErrorEvent = 8, TimerEvent = 16 };
	enum : uint16_t { PingID = 0xAFAF, PingLoadSize = 32, MaxPongTimeoutMs = 5000 };
	State sState;
	Error sError;
	EvtSource extEventSource;
	threads_queue_t waitingQueue;
	virtual_timer_t timer;
	struct Request
	{
		Request( IpAddress addr ) : addr( addr ),
			result( false ), processing( false ),
			seqNum( 0 ), sendingTime( 0 ), interval( 0 )
		{
			chBSemObjectInit( &sem, true );
		}
		IpAddress addr;
		bool result;
		bool processing;
		uint16_t seqNum;
		systime_t sendingTime;
		sysinterval_t interval;
		binary_semaphore_t sem;
	};
	Mutex mutex;
	NanoList< Request* > reqList;
	sysinterval_t pongTimeout;
	uint16_t seqNum;
	netconn* conn;
	volatile uint32_t recvCount;
};