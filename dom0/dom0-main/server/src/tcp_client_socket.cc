#include "tcp_client_socket.h"

#include <l4/dom0-main/communication_magic_numbers.h>
#include <unistd.h>

TcpClientSocket::TcpClientSocket(ip_addr_t address, int port)
{
	connected = false;
	targetSockaddr_in.sin_family = AF_INET;
	targetSockaddr_in.sin_port = htons(port);
	targetSockaddr_in.sin_addr.s_addr = address.addr;
}

TcpClientSocket::~TcpClientSocket()
{
	disconnect();
}

bool TcpClientSocket::isDisabled()
{
	return ntohs(targetSockaddr_in.sin_port) == 0;
}

int TcpClientSocket::connect()
{
	if (isDisabled())
		return 1;

	targetSocket = socket(PF_INET, SOCK_STREAM, 0);
	printf("socket created: %d\n", targetSocket);
	if (::connect(targetSocket, (struct sockaddr*) &targetSockaddr_in,
			sizeof(targetSockaddr_in)) == 0)
	{
		printf("connected\n");
		printf("%s\n", strerror(errno));
		connected = true;
		return 1;
	}
	else
		return -errno;

}

int TcpClientSocket::disconnect()
{
	if (isDisabled())
		return 1;

	close(targetSocket);
	connected = false;
	return 1;
}
