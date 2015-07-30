#pragma once

#include "tcp_client_socket.h"

#include <stdint.h>

//This class abstracts our "protocol"
//(e.g. sending of message type identifiers, string sizes, binary sizes etc.)
//so that one can use a simple and clean interface without knowing the
//"protocol" details.
class RemoteDom0Instance
{
private:
	TcpClientSocket remoteServerLuaSocket;
	TcpClientSocket remoteServerBinarySocket;

	//Load the contents of filename into buffer.
	//Note that buffer gets space allocated inside this function
	//(since we don't know the filesize beforehand) so that
	//this function can (and should) be called with an uninitialized
	//buffer. Buffer must be also free()'d from the caller
	//once it isn't needed anymore.
	static int32_t loadFile(const char* filename, char*& buffer);

public:
	RemoteDom0Instance(ip_addr_t address, int luaPort, int binaryPort);

	int connect();

	int disconnect();

	//Send a LUA command to the remote machine.
	int32_t executeLuaString(const char* string, int32_t size);
	int32_t executeLuaString(const char* string);

	//Sends a buffer containing an ELF binary
	int32_t sendElfBinaryFromMemory(char* data, size_t size,
			const char* remoteFilename);

	//Sends a file containing an ELF binary
	int32_t sendElfBinaryFromRom(const char* localFilename,
			const char* remoteFilename);
};

