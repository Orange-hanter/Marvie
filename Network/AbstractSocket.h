#pragma once

#include "Core/IODevice.h"
#include "IpAddress.h"

enum class SocketType { Unknown, Udp, Tcp };
enum class SocketState { Unconnected, Connecting, Connected, Closing };
enum class SocketError { NoError, ConnectionRefusedError, RemoteHostClosedError, HostNotFoundError, SocketResourceError, SocketTimeoutError, DatagramTooLargeError, NetworkError };
enum class SocketEventFlag { Error = 1, StateChanged = 2, InputAvailable = 4, OutputEmpty = 8 };

class AbstractSocket : public IODevice
{
public:
	AbstractSocket() : userData( nullptr ) {}

	virtual bool bind( uint16_t port ) = 0;
	virtual bool connect( const char* hostName, uint16_t port ) = 0;
	virtual bool connect( IpAddress address, uint16_t port ) = 0;
	virtual void disconnect() = 0;

	virtual SocketType type() = 0;
	virtual SocketState state() = 0;
	virtual SocketError socketError() const = 0;

public:
	void* userData;
};