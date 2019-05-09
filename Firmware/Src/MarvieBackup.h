#include "Core/DateTime.h"
#include "Network/IpAddress.h"
#include "Crc32HW.h"
#include "Core/Mutex.h"

struct MarvieBackup
{
	static inline MarvieBackup* instance() { return ( MarvieBackup* )BKPSRAM_BASE; }

	uint64_t marker;
	uint32_t version;
	struct Settings
	{
		uint32_t crc;
		struct Flags
		{
			uint32_t ethernetDhcp : 1;
			uint32_t gsmEnabled : 1;
			uint32_t sntpClientEnabled : 1;
		} flags;
		struct AccountPasswords
		{
			char adminPassword[31 + 1];
			char observerPassword[31 + 1];
		} passwords;
		struct Ethernet
		{
			IpAddress ip;
			uint32_t netmask;
			IpAddress gateway;
		} eth;
		struct Gsm
		{
			uint16_t pinCode;
			char apn[31 + 1];
		} gsm;
		struct DateTime
		{
			int64_t lastSntpSync;
			uint32_t timeZone;
		} dateTime;

		bool isValid();
		void setValid();
		void reset();
	} settings;
	struct FailureDesc
	{
		enum Type : int32_t
		{
			None = 0,
			SystemHalt,
			Reset = 2,
			NMI,
			HardFault,
			MemManage,
			BusFault,
			UsageFault,
		} type;
		union Union
		{
			struct FailureDesc
			{
				uint32_t flags;
				uint32_t busAddress;
				uint32_t pc;
				uint32_t lr;
			} failure;
			char msg[63 + 1];
		} u;
		uint32_t threadAddress;
		char threadName[31 + 1];

		struct PowerDown
		{
			bool power;
			bool detected;
			DateTime dateTime;
		} pwrDown;

		void reset();
	} failureDesc;

	static constexpr uint64_t correctMarker = 0x0123456789ABCDEF;
	void init();
	inline bool isMarkerCorrect() { return marker == correctMarker; }
	void reset();
	void acquire();
	void release();

private:
	static Mutex mutex;
};