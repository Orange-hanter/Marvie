#pragma once

#include "AbstractSocket.h"

class AbstractTcpSocket : public AbstractSocket
{
public:
	SocketType type() final override;
};