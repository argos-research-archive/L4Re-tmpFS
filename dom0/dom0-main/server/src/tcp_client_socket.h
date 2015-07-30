#pragma once

#include "tcp_socket.h"

#include <lwip/ip4_addr.h>

class TcpClientSocket: public TcpSocket
{
public:
	TcpClientSocket(ip_addr_t address, int port);

	virtual ~TcpClientSocket();

	int connect();

	int disconnect();

	bool isDisabled();

private:

};
