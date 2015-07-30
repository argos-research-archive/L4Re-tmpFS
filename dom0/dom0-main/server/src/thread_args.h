#pragma once

#include <l4/sys/capability>
#include <l4/re/dataspace>
#include <lwip/ip_addr.h>
#include "lua_ipc_client.h"

//Since pthread_create only accepts functions with a single void* argument,
//all arguments need to be packed into a structure.

struct ClientArgs
{
	ip_addr_t address;
	int luaPort;
	int binaryPort;
	LuaIpcClient* luaIpcClient;
};

struct ServerArgs
{
	int luaPort;
	int binaryPort;
	LuaIpcClient* luaIpcClient;
};

struct ListenerArgs
{
	int port;
	enum Type
	{
		binary, lua
	};
	Type type;
};
