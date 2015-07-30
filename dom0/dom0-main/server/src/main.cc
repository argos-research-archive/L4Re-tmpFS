/*
 * Need to include this file before others.
 * Sets our byteorder.
 */
extern "C"
{
#include "arch/cc.h"
}

#include <l4/re/dataspace>
#include <l4/util/util.h>
#include <l4/ankh/client-c.h>
#include <l4/ankh/session.h>
#include <l4/ankh/lwip-ankh.h>
#include <lwip/ip4_addr.h>
#include <arpa/inet.h>
#include <netif/etharp.h>
#include <lwip-util.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <pthread-l4.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "remote_dom0_instance.h"
#include "dom0_server.h"
#include "thread_args.h"
#include "lua_ipc_client.h"
#include "tcp_client_socket.h"
#include <l4/dom0-main/communication_magic_numbers.h>
#include <l4/dom0-main/ipc_protocol.h>


#define PATH_LEN 255
#define GETOPT_LIST_END { 0, 0, 0, 0 }

enum options
{
	BUF_SIZE,
	SHM_NAME,
	ENABLE_CLIENT,
	LISTEN_ADDRESS,
	LUA_LISTEN_PORT,
	BINARY_LISTEN_PORT,
	GATEWAY,
	NETMASK,
	TARGET_ADDRESS,
	LUA_TARGET_PORT,
	BINARY_TARGET_PORT
};

extern "C"
{
extern err_t
ankhif_init(struct netif*);
}

static ankh_config_info cfg =
{ 1024, L4_INVALID_CAP, L4_INVALID_CAP, "" };

//TODO: remove hwbit and its usage
//For now, we use it to differentiate between our instances while still only
//using this single program. See also the TODO for tmpIpAddr.
u8_t hwbit;



static void* clientDemo(void* args)
{


	printf("Client mode\n");

	sleep(2);


	//luaIpc processes LUA commands for the local machine
	LuaIpcClient& luaIpc = *((ClientArgs*) args)->luaIpcClient;

	//remoteDom0 processes LUA commands for the remote machine
	//and receives binaries
	RemoteDom0Instance remoteDom0(((ClientArgs*) args)->address,
			((ClientArgs*) args)->luaPort,((ClientArgs*) args)->binaryPort);
	remoteDom0.connect();

	sleep(2);

	//DEMO CODE STARTS HERE
	printf("********DEMO*******\n");


	//VERY hacky, but just for testing and demonstration purposes:
	//We use hwbit for differentiating between three client instances
	//so that they can execute different commands while still using only
	//this same one single program.
	//These three different paths are then executed on three different
	//(emulated) machines, since they have different MAC addresses.
	if(hwbit%10==2)
	{
		printf("Client 1\n");

		printf("start test task.\n");
		sleep(2);
		remoteDom0.executeLuaString("hello1 = L4.default_loader:start({log = {\"hello1\", \"c\"}},\"rom/hello\");");

		sleep(3);

		printf("send example binary.\n");
		sleep(2);
		remoteDom0.sendElfBinaryFromRom("rom/test", NETWORK_BINARY_PATH "test");
		printf("sent binary. starting of such binaries not yet implemented.\n");


	}
	else if(hwbit%10==3)
	{
		printf("Client 2\n");

		printf("start test task.\n");
		sleep(2);
		remoteDom0.executeLuaString("hello2 = L4.default_loader:start({log = {\"hello2\", \"y\"}},\"rom/hello\");");

		sleep(10);

		printf("kill \"foreign\" test task.\n");
		sleep(2);
		remoteDom0.executeLuaString("hello1:kill();");

		sleep(10);

		printf("kill \"own\" test task.\n");
		sleep(2);
		remoteDom0.executeLuaString("hello2:kill();");

	}
	else if(hwbit%10==4)
	{
		printf("Client 3\n");


		sleep(7);

		printf("start LOCAL test task.\n");
		sleep(2);
		luaIpc.executeString("hello = L4.default_loader:start({log = {\"hi1\", \"r\"}},\"rom/hello\");");

		sleep(10);

		printf("kill LOCAL test task.\n");
		sleep(2);
		luaIpc.executeString("hello:kill();");
	}
	printf("********END OF DEMO*******\n");


	l4_sleep_forever();
	return 0;

}

int main(int argc, char **argv)
{
	//Initialize stuff
	LuaIpcClient luaIpc("lua_ipc");
	struct netif my_netif;
	int luaListenPort = 0;
	int binaryListenPort = 0;
	int luaTargetPort = 0;
	int binaryTargetPort = 0;
	//ip_addr_t localAddress;
	ip_addr_t targetAddress;
	ip_addr_t gateway;
	ip_addr_t netmask;
	//localAddress.addr = 0;
	targetAddress.addr = 0;
	gateway.addr = 0;
	netmask.addr = 0;
	pthread_t clientThread = NULL;
	ClientArgs clientArgs;
	ServerArgs serverArgs;
	bool enableClient = false;

	luaIpc.init();
	if (l4ankh_init())
		return 1;
	l4_cap_idx_t c = pthread_getl4cap(pthread_self());
	cfg.send_thread = c;
	cfg.recv_thread = c;


	//Process arguments, check for invalid values
	static struct option long_opts[] =
	{
	{ "bufsize", required_argument, 0, BUF_SIZE },
	{ "shm", required_argument, 0, SHM_NAME },
	{ "enable-client", no_argument, 0, ENABLE_CLIENT },
	{ "listen-address", required_argument, 0, LISTEN_ADDRESS },
	{ "lua-listen-port", required_argument, 0, LUA_LISTEN_PORT },
	{ "binary-listen-port", required_argument, 0, BINARY_LISTEN_PORT },
	{ "gateway", required_argument, 0, GATEWAY },
	{ "netmask", required_argument, 0, NETMASK },
	{ "target-address", required_argument, 0, TARGET_ADDRESS },
	{ "lua-target-port", required_argument, 0, LUA_TARGET_PORT },
	{ "binary-target-port", required_argument, 0, BINARY_TARGET_PORT },
	GETOPT_LIST_END };

	while (1)
	{
		int optind = 0;
		int opt = getopt_long(argc, argv, "", long_opts,
				&optind);
		printf("getopt: %d\n", opt);

		if (opt == -1)
			break;

		switch (opt)
		{
		case BUF_SIZE:
			printf("buf size: %d\n", atoi(optarg));
			cfg.bufsize = atoi(optarg);
			break;
		case SHM_NAME:
			printf("shm name: %s\n", optarg);
			snprintf(cfg.shm_name, CFG_SHM_NAME_SIZE, "%s", optarg);
			break;
		case ENABLE_CLIENT:
			printf("Client enabled\n");
			enableClient = true;
			break;
		/*case LISTEN_ADDRESS:
			printf("Interface and listen address: %s\n", optarg);
			if (!inet_aton(optarg, (in_addr*) &localAddress))
			{
				printf("Not a valid IPv4 address. Exiting.\n");
				return -1;
			}
			break;*/
		case TARGET_ADDRESS:
			printf("Target address: %s\n", optarg);
			if (!inet_aton(optarg, (in_addr*) &targetAddress))
			{
				printf("Not a valid IPv4 address. Exiting.\n");
				return -1;
			}
			break;
		case GATEWAY:
			printf("Gateway address: %s\n", optarg);
			if (!inet_aton(optarg, (in_addr*) &gateway))
			{
				printf("Not a valid IPv4 address. Exiting.\n");
				return -1;
			}
			break;
		case NETMASK:
			printf("Netmask: %s\n", optarg);
			if (!inet_aton(optarg, (in_addr*) &netmask))
			{
				printf("Not a valid netmask. Exiting.\n");
				return -1;
			}
			break;
		case LUA_LISTEN_PORT:
			printf("LUA Listen port: %d\n", atoi(optarg));
			luaListenPort = atoi(optarg);
			break;
		case BINARY_LISTEN_PORT:
			printf("Binary Listen port: %d\n", atoi(optarg));
			binaryListenPort = atoi(optarg);
			break;
		case LUA_TARGET_PORT:
			printf("LUA Target port: %d\n", atoi(optarg));
			luaTargetPort = atoi(optarg);
			break;
		case BINARY_TARGET_PORT:
			printf("Binary Target port: %d\n", atoi(optarg));
			binaryTargetPort = atoi(optarg);
			break;
		default:
			break;
		}
	}
	if (cfg.bufsize == 0)
	{
		printf("No valid buffer size specified. Exiting.\n");
		return -1;
	}
	if (cfg.shm_name[0] == '\0')
	{
		printf("Empty shm name. Exiting.\n");
		return -1;
	}
	/*if (localAddress.addr == 0)
	{
		printf("No valid local address specified. Exiting.\n");
		return -1;
	}*/
	if (netmask.addr == 0)
	{
		printf("No valid netmask specified. Exiting.\n");
		return -1;
	}
	if (gateway.addr == 0)
	{
		printf("No valid gateway specified. Exiting.\n");
		return -1;
	}
	if (luaListenPort == 0)
	{
			printf("No valid LUA bind port specified. Exiting.\n");
			return -1;
	}
	if (binaryListenPort == 0)
	{
			printf("No valid binary bind port specified. Exiting.\n");
			return -1;
	}
	if (enableClient)
	{
		if (targetAddress.addr == 0)
		{
			printf("No valid target address specified. Exiting.\n");
			return -1;
		}
		if (luaTargetPort == 0)
		{
			printf("No valid target LUA port specified. Exiting.\n");
			return -1;
		}
		if (binaryTargetPort == 0)
		{
			printf("No valid target binary port specified. Exiting.\n");
			return -1;
		}
	}


	//We don't need to mount manually. This is done automatically
	//by linking our app with libmount and specifying a fstab file.
	//mount("/","net-files","tmpfs",0,0);
	//perror("mount");


	// Start the TCP/IP thread & init remaining networking stuff
	tcpip_init(NULL, NULL);
	//TODO: Replace tmpIpAddr with localAddress again and do the address
	//stuff somewhere else (e.g. in the configuration files),
	//but for now this enables us to have a single program for different
	//execution paths. See also the TODO for hwbit.
	ip_addr_t tmpIpAddr;
	IP4_ADDR(&tmpIpAddr, 0,0,0,0);
	my_netif.flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
	struct netif *n = netif_add(&my_netif, /*&localAddress*/ &tmpIpAddr, &netmask, &gateway,
			&cfg, // configuration state
			ankhif_init,
			//for multithreaded network communication, use
			tcpip_input
			//for singlethreaded network communication only, use instead
			//ethernet_input
	);

	printf("netif_add: %p (%p)\n", n, &my_netif);
	assert(n == &my_netif);

	IP4_ADDR(&tmpIpAddr, 192,168,0,(my_netif.hwaddr[5]%240)+1);
	hwbit = my_netif.hwaddr[5];
	netif_set_ipaddr(&my_netif,&tmpIpAddr);
	netif_set_default(&my_netif);
	netif_set_up(&my_netif);
	while (!netif_is_up(&my_netif))
		sleep(1);
	printf("Network interface is up.\n");
	printf("IP: ");
	lwip_util_print_ip(&my_netif.ip_addr);
	printf("\n");
	printf("GW: ");
	lwip_util_print_ip(&my_netif.gw);
	printf("\n");

	if (enableClient)
	{
		//start the client demo in a new thread
		clientArgs.address = targetAddress;
		clientArgs.luaPort = luaTargetPort;
		clientArgs.binaryPort = binaryTargetPort;
		clientArgs.luaIpcClient = &luaIpc;
		pthread_create(&clientThread, NULL, clientDemo, &clientArgs);
	}

	{
		//start the dom0 server thread
		//We don't need an address here, since we just listen on all interfaces
		//serverArgs.address = /*localAddress*/ tmpIpAddr;
		serverArgs.luaPort = luaListenPort;
		serverArgs.binaryPort = binaryListenPort;
		serverArgs.luaIpcClient = &luaIpc;
		dom0Server(&serverArgs);
	}

	//main thread must not terminate because of all the other threads
	l4_sleep_forever();
	return 0;
}
