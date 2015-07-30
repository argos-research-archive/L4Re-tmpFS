#include "remote_dom0_instance.h"

#include <l4/dom0-main/communication_magic_numbers.h>

//Load a file into a memory block
int32_t RemoteDom0Instance::loadFile(const char* filename, char*& buffer)
{
	//Copy binary into memory
	FILE *fp;
	size_t size, read;
	fp = fopen(filename, "r");
	if (!fp)
	{
		perror("fopen");
		return -1;
	}

	//Get filesize
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	printf("Reading file\nsize:\t%u Bytes\n", size);

	//Allocate memory and copy file content into buffer
	buffer = (char*) malloc(size);
	if (!buffer)
	{
		perror("Fehler");
		return -1;
	}
	read = fread(buffer, 1, size, fp);
	printf("read:\t%u Bytes\n", read);
	fclose(fp);
	if (read == size)
		printf("File loaded into memory.\n");
	return size;
}

RemoteDom0Instance::RemoteDom0Instance(ip_addr_t address, int luaPort,
		int binaryPort) :
		remoteServerLuaSocket(address, luaPort), remoteServerBinarySocket(
				address, binaryPort)
{

}

int RemoteDom0Instance::connect()
{
	return remoteServerLuaSocket.connect() & remoteServerBinarySocket.connect();
}

int RemoteDom0Instance::disconnect()
{
	return remoteServerBinarySocket.disconnect()
			& remoteServerBinarySocket.disconnect();
}

//Send a LUA command to the remote machine.
int32_t RemoteDom0Instance::executeLuaString(const char* string)
{
	return executeLuaString(string, strlen(string) + 1);
}

int32_t RemoteDom0Instance::executeLuaString(const char* string, int32_t size)
{
	if (remoteServerLuaSocket.isDisabled())
		return 0;

	int32_t result;

	//Send the size
	if (remoteServerLuaSocket.sendInt32_t(size) < 1)
		return -errno;

	//Send the command.
	if (remoteServerLuaSocket.sendData((void*) string, size) < 1)
		return -errno;

	//The server responds if its Ned got the whole message.
	//This tells nothing about the success of the
	//execution of the command.
	if (remoteServerLuaSocket.receiveInt32_t(result) < 1)
		return -errno;

	return result;
}

//Sends a buffer containing an ELF binary
int32_t RemoteDom0Instance::sendElfBinaryFromMemory(char* data, size_t size,
		const char* remoteFilename)
{
	if (remoteServerBinarySocket.isDisabled())
		return 0;

	int32_t message;

	//Send the filename our binary should get on the remote machine
	if (remoteServerBinarySocket.sendInt32_t(
			(int32_t) strlen(remoteFilename) + 1) < 1)
		return -errno;
	if (remoteServerBinarySocket.sendData((void*) remoteFilename,
			strlen(remoteFilename) + 1) < 1)
		return -errno;

	//Send the binary size
	if (remoteServerBinarySocket.sendInt32_t((int32_t) size) < 1)
		return -errno;

	//Wait for the "GO" from the server
	if (remoteServerBinarySocket.receiveInt32_t(message) < 1)
		return -errno;

	//Send it
	if (remoteServerBinarySocket.sendData(data, (int32_t) size) < 1)
		return -errno;
	else
		return size;
}

//Sends a file containing an ELF binary
int32_t RemoteDom0Instance::sendElfBinaryFromRom(const char* localFilename,
		const char* remoteFilename)
{
	if (remoteServerBinarySocket.isDisabled())
		return 0;

	char* data = NULL;
	int32_t message = loadFile(localFilename, data);
	message = sendElfBinaryFromMemory(data, message, remoteFilename);
	free(data);
	return message;
}
