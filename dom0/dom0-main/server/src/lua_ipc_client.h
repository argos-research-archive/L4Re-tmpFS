#pragma once

#include <l4/sys/capability>
#include <l4/sys/l4int.h>
#include <stddef.h>

//An object of this class can connect to a Ned task
//running on the same machine.
//Thus, it is capable of executing LUA commands locally.
class LuaIpcClient
{
public:
	LuaIpcClient(const char* capName);

	bool init();

	//Send LUA command to Ned. Returns true, if ned received the string successfully.
	bool executeString(const char* cmd, size_t length);
	bool executeString(const char* cmd);

private:
	L4::Cap<void> luaIpcServer;
	const char* name;

};
