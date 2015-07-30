#include "tcp_socket.h"

#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <lwip/ip_addr.h>

//Receive data from the socket and write it into data.
ssize_t receiveData(void* data, size_t size, int targetSocket)
{
	printf("DEBUG: Receiving %i bytes.\n", size);
	ssize_t result = 0;
	ssize_t position = 0;
	//Because read() might actually read less than size bytes
	//before it returns, we call it in a loop
	//until size bytes have been read.
	do
	{
		result = read(targetSocket, (char*) data + position, size - position);
		if (result < 1)
			return -errno;
		position += result;

	} while ((size_t) position < size);
	return position;
}

//convenience function
ssize_t receiveInt32_t(int32_t& data, int targetSocket)
{
	return receiveData(&data, sizeof(data), targetSocket);
}

//Send data from buffer data with size size to the socket.
ssize_t sendData(void* data, size_t size, int targetSocket)
{
	printf("DEBUG: Sending %i bytes.\n", size);
	ssize_t result = 0;
	ssize_t position = 0;
	//Because write() might actually write less than size bytes
	//before it returns, we call it in a loop
	//until size bytes have been written.
	do
	{
		result = write(targetSocket, (char*) data + position, size - position);
		if (result < 1)
			return -errno;
		position += result;

	} while ((size_t) position < size);
	return position;
}

//convenience function
ssize_t sendInt32_t(int32_t data, int targetSocket)
{
	return sendData(&data, sizeof(data), targetSocket);
}

//Receive data from the socket and write it into data.
ssize_t TcpSocket::receiveData(void* data, size_t size)
{
	return ::receiveData(data, size, targetSocket);
}

//convenience function
ssize_t TcpSocket::receiveInt32_t(int32_t& data)
{
	return ::receiveInt32_t(data, targetSocket);
}

//Send data from buffer data with size size to the socket.
ssize_t TcpSocket::sendData(void* data, size_t size)
{
	return ::sendData(data, size, targetSocket);
}

//convenience function
ssize_t TcpSocket::sendInt32_t(int32_t data)
{
	return ::sendInt32_t(data, targetSocket);
}
