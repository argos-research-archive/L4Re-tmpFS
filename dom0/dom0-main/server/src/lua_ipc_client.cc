#include "lua_ipc_client.h"

#include <l4/re/util/cap_alloc>
#include <l4/re/env>
#include <l4/cxx/ipc_stream>
#include <l4/dom0-main/communication_magic_numbers.h>
#include <l4/dom0-main/ipc_protocol.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

LuaIpcClient::LuaIpcClient(const char* capName) :
		luaIpcServer(L4_INVALID_CAP)
{
	this->name = capName;
}

bool LuaIpcClient::init()
{
	luaIpcServer = L4Re::Env::env()->get_cap<void>(name);
	if (!luaIpcServer.is_valid())
	{
		printf("Could not get IPC gate capability for Ned/LUA!\n");
		return false;
	}
	return true;
}

//Send LUA command to Ned. Returns true, if Ned received the string successfully.
bool LuaIpcClient::executeString(const char* cmd, size_t length)
{
	int32_t result;
	L4::Ipc::Iostream s(l4_utcb());
	//Before the actual command the length of the command is sent
	//so that Ned can later check if it received the whole message.
	//If it has, it answers with LUA_OK.
	//Note however that it does not say anything about
	//the success of the LUA command execution,
	//only of the reception of the command.
	s << L4::Opcode(Opcode::executeLuaLine) << length << cmd;
	result = l4_error(s.call(luaIpcServer.cap(), Protocol::LuaDom0));
	if (result)
		return false; // failure

	s >> result;
	return result == LUA_OK;
}
bool LuaIpcClient::executeString(const char* cmd)
{
	return executeString((char*) ((cmd)), strlen(cmd) + 1);
}
