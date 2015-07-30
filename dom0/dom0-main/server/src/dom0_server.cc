#include "dom0_server.h"

#include "lua_ipc_client.h"
#include "thread_args.h"
#include "tcp_socket.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread-l4.h>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/util/util.h>
#include <l4/dom0-main/communication_magic_numbers.h>
#include <l4/dom0-main/ipc_protocol.h>
#include <l4/fs/l4fs.h>
#include <lwip-util.h>

static LuaIpcClient* luaIpc;

//Receives a file from the network of size size and
//writes it to the file specified by fd
ssize_t receiveNetworkBinary(int fd, size_t size, int targetSocket)
{
	size_t leftToReceive = size;
	size_t written = 0;
	size_t received = 0;
	const size_t maxSize = L4fs::Max_rw_buffer_size;
	printf("max chunk size %u\n", maxSize);
	char* rwBuffer = (char*) malloc(maxSize);

	//This loop is needed in case that the file
	//to be received is bigger than our buffer
	while (leftToReceive > 0)
	{
		printf("left: %u\n", leftToReceive);
		received = receiveData(rwBuffer,
				(maxSize < leftToReceive ? maxSize : leftToReceive),
				targetSocket);
		leftToReceive -= received;
		written = 0;
		printf("received %u\n", received);

		//Often, multiple write() calls are needed
		//to write all the data
		while (written < received)
		{
			written += write(fd, rwBuffer + written, received - written);
			perror("write");
			printf("written %u Bytes\n", written);
		}
	}

	return size;
}

static void* luaConnectionHandler(void* socket_desc)
{
	printf("New instance of the LUA connection handler started.\n");
	int fd = *(int*) socket_desc;
	free(socket_desc);

	//Binary commands from the communication partner will be stored here
	int32_t message = 0;

	//LUA commands from the communication partner and paths will be stored here
	char buffer[BUF_LEN];

	while (true)
	{
		//The first four bytes contain the length
		//of the LUA string (including terminating zero)
		NETCHECK_LOOP(receiveInt32_t(message, fd));
		//Now, receive the LUA string
		NETCHECK_LOOP(receiveData(buffer, message, fd));
		printf("LUA received: %s\n", buffer);
		//And send it to our local Ned instance.
		if (luaIpc->executeString(buffer))
			message = LUA_OK;
		else
			message = LUA_ERROR;
		//Answer, if Ned received the message successfully
		//or not (e.g. because the string was too long)
		NETCHECK_LOOP(sendInt32_t(message, fd));
	}

	//Close the socket
	close(fd);

	return 0;
}

static void* binaryConnectionHandler(void* socket_desc)
{
	printf("New instance of the binary connection handler started.\n");
	int fd = *(int*) socket_desc;
	free(socket_desc);
	int message = 1;
	char buffer[BUF_LEN];

	while (true)
	{
		//Get the desired filename (path) of the to be received binary
		NETCHECK_LOOP(receiveInt32_t(message, fd));
		NETCHECK_LOOP(receiveData(buffer, message, fd));

		//Create the respective file
		int file = open(buffer, O_WRONLY | O_CREAT);
		perror("open");

		//Get the size of the binary.
		NETCHECK_LOOP(receiveInt32_t(message, fd));
		printf("Binary size will be %i Bytes.\n", message);

		//Our partner can now start sending.
		NETCHECK_LOOP(sendInt32_t(GO_SEND,fd));

		printf("before receiving\n");
		//Get it...
		NETCHECK_LOOP(receiveNetworkBinary(file, message, fd));
		printf("Binary received.\n");

	}

	//Close the socket
	close(fd);

	return 0;
}

static void* serverCombined(void* args)
{

	//Initialize socket stuff
	int* new_sock;
	void*(*targetFunc)(void*);
	if (((ListenerArgs*) args)->type == ListenerArgs::lua)
		targetFunc = luaConnectionHandler;
	else
		targetFunc = binaryConnectionHandler;
	int err;
	struct sockaddr_in in, clnt;
	in.sin_family = AF_INET;
	in.sin_port = htons(((struct ListenerArgs*) args)->port);
	in.sin_addr.s_addr = INADDR_ANY;

	int sock = socket(PF_INET, SOCK_STREAM, 0);
	printf("socket created: %d\n", sock);

	err = bind(sock, (struct sockaddr*) &in, sizeof(in));
	printf("bound to addr: %d\n", err);

	err = listen(sock, 10);
	printf("listen(): %d\n", err);

	socklen_t clnt_size = sizeof(clnt);

	int newfd;

	//Each time a new connection arrives, start a new handler thread
	//and pass it the new socket
	while (true)
	{
		newfd = accept(sock, (struct sockaddr*) &clnt, &clnt_size);
		new_sock = (int*) malloc(sizeof(int));
		*new_sock = newfd;

		pthread_t* sniffer_thread;
		sniffer_thread = (pthread_t*) (calloc(1, sizeof(pthread_t)));
		pthread_create(sniffer_thread, NULL, targetFunc, (void*) new_sock);

		printf("Handler assigned.\n");

	}

	return NULL;

}

//This function is the core of dom0 (server).
//It starts the two listener threads for
//LUA-commands and binaries.
void* dom0Server(void* args)
{

	struct ListenerArgs binaryListenerArgs;
	binaryListenerArgs.port = ((ServerArgs*) args)->binaryPort;
	binaryListenerArgs.type = ListenerArgs::binary;

	struct ListenerArgs luaListenerArgs;
	luaListenerArgs.port = ((ServerArgs*) args)->luaPort;
	luaListenerArgs.type = ListenerArgs::lua;

	luaIpc = ((ServerArgs*) args)->luaIpcClient;

	pthread_t luaThread;
	pthread_create(&luaThread, NULL, serverCombined, &luaListenerArgs);

	pthread_t binaryThread;
	pthread_create(&binaryThread, NULL, serverCombined, &binaryListenerArgs);

	return NULL;
}
