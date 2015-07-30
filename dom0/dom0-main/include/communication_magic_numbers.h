#pragma once

//these numbers are used in network communication
//to identify the type of the payload


//Possible answers to LUA commands:
#define LUA_OK 0x10A900D
#define LUA_ERROR 0x10ABAD

//Binary-handling Control messages:
//server tells client that he's ready to receive
#define GO_SEND 0x90


//Path for network-received binaries
#define NETWORK_BINARY_PATH "/net-files/"