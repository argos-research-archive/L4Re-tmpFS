#pragma once

#include <sys/types.h>

#define BUF_LEN 1024

//This function is the core of dom0.
//It receives commands from a connected
//dom0 client (on another machine), and processes
//and answers them accordingly.
void* dom0Server(void* args);
