/*
 * Need to include this file before others.
 * Sets our byteorder.
 */
#include "arch/cc.h"

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread-l4.h>
#include <l4/util/util.h>
#include <l4/ankh/client-c.h>
#include <l4/ankh/lwip-ankh.h>
#include <l4/ankh/session.h>
#include <unistd.h>
#include <netdb.h>
#include "netif/etharp.h"
#include <lwip-util.h>
#include "lwip/dhcp.h"

#define GETOPT_LIST_END { 0, 0, 0, 0 }

#define PTHREAD

#define PORT_OFFSET 16
#define IP_OFFSET 32

enum options
{
	BUF_SIZE, SHM_NAME
};

/* Network interface variables */
ip_addr_t ipaddr, netmask, gw;
struct netif netif;
/* Set network address variables */
#if 0
IP4_ADDR(&gw, 192,168,0,1);
IP4_ADDR(&ipaddr, 192,168,0,2);
IP4_ADDR(&netmask, 255,255,255,0);
#endif

struct netif my_netif;
extern err_t ankhif_init(struct netif*);

static ankh_config_info cfg =
{ 1024, L4_INVALID_CAP, L4_INVALID_CAP, "" };

static void *connection_handler(void *socket_desc)
{
	printf("Hi from handler 1\n");
	int fd = *(int*) socket_desc;
	char buf[1024];

	int err = 1;
	while (err > 0)
	{
		err = read(fd, buf, sizeof(buf));
		printf("Message from ");
		lwip_util_print_ip((ip_addr_t*) ((int*) socket_desc + IP_OFFSET));
		printf(": %s\n", buf);
		if (err <= 0)
			break;
		err = write(fd, buf, strlen(buf) + 1);
	}
	printf("connection terminated\n");

	//Free the socket pointer
	free(socket_desc);

	return 0;
}

static void *connection_handler2(void *socket_desc)
{
	printf("Hi from handler 2\n");
	int fd = *(int*) socket_desc;
	char buf[1024];

	int err = 1;
	while (err > 0)
	{
		err = read(fd, buf, sizeof(buf));
		printf("Message from ");
		lwip_util_print_ip((ip_addr_t*) ((int*) socket_desc + IP_OFFSET));
		printf(": %s\n", buf);
		if (err <= 0)
			break;
		err = write(fd, buf, strlen(buf) + 1);
	}
	printf("connection terminated\n");

	//Free the socket pointer
	free(socket_desc);

	return 0;
}

static void* server(void* arg)
{
	int* new_sock;
	int err, fd;
	struct sockaddr_in in, clnt;
	in.sin_family = AF_INET;
	int port = 1111;
	in.sin_port = htons(port);
	in.sin_addr.s_addr = my_netif.ip_addr.addr;

	int sock = socket(PF_INET, SOCK_STREAM, 0);
	printf("Listener 1: socket created: %d\n", sock);

	err = bind(sock, (struct sockaddr*) &in, sizeof(in));
	printf("Listener 1: bound to addr: %d\n", err);

	err = listen(sock, 10);
	printf("Listener 1: listen(): %d\n", err);

	//For each new incomming connection,
	//start a new thread and pass it the new socket
	for (;;)
	{
		socklen_t clnt_size = sizeof(clnt);
		fd = accept(sock, (struct sockaddr*) &clnt, &clnt_size);

		printf("Listener 1: Got connection from  ");
		lwip_util_print_ip((ip_addr_t*) &clnt.sin_addr);
		printf("\n");

		new_sock = malloc(sizeof(int));
		*new_sock = fd;

		pthread_t* sniffer_thread;
		sniffer_thread = (pthread_t*) (calloc(128, sizeof(pthread_t)));
		*((int*) new_sock + IP_OFFSET) = clnt.sin_addr.s_addr;
		pthread_create(sniffer_thread, NULL, connection_handler,
				(void*) new_sock);

		printf("Listener 1: Handler assigned\n");
	}

	return NULL;
}

static void* server2(void* arg)
{
	int* new_sock;
	int err, fd;
	struct sockaddr_in in, clnt;
	in.sin_family = AF_INET;
	int port = 9999;
	in.sin_port = htons(port);
	in.sin_addr.s_addr = my_netif.ip_addr.addr;

	int sock = socket(PF_INET, SOCK_STREAM, 0);
	printf("Listener 2: socket created: %d\n", sock);

	err = bind(sock, (struct sockaddr*) &in, sizeof(in));
	printf("Listener 2: bound to addr: %d\n", err);

	err = listen(sock, 10);
	printf("Listener 2: listen(): %d\n", err);

	//For each new incomming connection,
	//start a new thread and pass it the new socket
	for (;;)
	{
		socklen_t clnt_size = sizeof(clnt);
		fd = accept(sock, (struct sockaddr*) &clnt, &clnt_size);

		printf("Listener 2: Got connection from  ");
		lwip_util_print_ip((ip_addr_t*) &clnt.sin_addr);
		printf("\n");

		new_sock = malloc(sizeof(int));
		*new_sock = fd;

		pthread_t* sniffer_thread;
		sniffer_thread = (pthread_t*) (calloc(128, sizeof(pthread_t)));
		*((int*) new_sock + IP_OFFSET) = clnt.sin_addr.s_addr;
		pthread_create(sniffer_thread, NULL, connection_handler2,
				(void*) new_sock);

		printf("Listener 2: Handler assigned\n");
	}

	return NULL;
}

int main(int argc, char **argv)
{
	IP4_ADDR(&gw, 192, 168, 0, 1);
	IP4_ADDR(&ipaddr, 192, 168, 0, 2);
	IP4_ADDR(&netmask, 255, 255, 255, 0);

	if (l4ankh_init())
		return 1;

	l4_cap_idx_t c = pthread_getl4cap(pthread_self());
	cfg.send_thread = c;

	static struct option long_opts[] =
	{
	{ "bufsize", 1, 0, BUF_SIZE },
	{ "shm", 1, 0, SHM_NAME },
	GETOPT_LIST_END };

	while (1)
	{
		int optind = 0;
		int opt = getopt_long(argc, argv, "b:s:", long_opts, &optind);
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
		default:
			break;
		}
	}

	// Start the TCP/IP thread & init stuff
	tcpip_init(NULL, NULL);
	struct netif *n = netif_add(&my_netif, &ipaddr, &netmask, &gw, &cfg, // configuration state
			ankhif_init,
			//ethernet_input
			tcpip_input);

	printf("netif_add: %p (%p)\n", n, &my_netif);
	assert(n == &my_netif);

	netif_set_default(&my_netif);
	netif_set_up(&my_netif);

	while (!netif_is_up(&my_netif))
		l4_sleep(1000);
	printf("Network interface is up.\n");

	printf("IP: ");
	lwip_util_print_ip(&my_netif.ip_addr);
	printf("\n");
	printf("GW: ");
	lwip_util_print_ip(&my_netif.gw);
	printf("\n");

	pthread_t thread;
	pthread_create(&thread, NULL, server, NULL);
	server2(NULL);

	l4_sleep_forever();
	return 0;
}

