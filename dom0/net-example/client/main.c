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

enum options
{
	BUF_SIZE,
	SHM_NAME
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

static ankh_config_info cfg = { 1024, L4_INVALID_CAP, L4_INVALID_CAP, "" };



static void client(void)
{
	int err;
	struct sockaddr_in in;
	in.sin_family = AF_INET;
	in.sin_port = htons(1111);
	ip_addr_t target;
	IP4_ADDR(&target,192,168,0,2);
	in.sin_addr.s_addr = target.addr;

	int sock = socket(PF_INET, SOCK_STREAM, 0);
	printf("socket created: %d\n", sock);

	err = connect(sock, (struct sockaddr*)&in, sizeof(in));
	printf("connected to addr: %d\n", err);

	char buf[1024];

	strcpy(buf,"TEST ON PORT 1111 A");;

	
	sleep(1); //otherwise wont work
	for (;;) {

		err = write(sock, buf, strlen(buf));
		printf("Written to fd %d (result %d)\n", sock, err);
		err = read(sock, buf, sizeof(buf));
		printf("Read from fd %d (result %d)\n", sock, err);
		printf("%s\n", buf);

		buf[18]++;
		sleep(2);
	}
}


static void client2(void)
{
	int err;
	struct sockaddr_in in;
	in.sin_family = AF_INET;
	in.sin_port = htons(9999);
	ip_addr_t target;
	IP4_ADDR(&target,192,168,0,2);
	in.sin_addr.s_addr = target.addr;

	int sock = socket(PF_INET, SOCK_STREAM, 0);
	printf("socket created: %d\n", sock);

	err = connect(sock, (struct sockaddr*)&in, sizeof(in));
	printf("connected to addr: %d\n", err);

	char buf[1024];

	strcpy(buf,"TEST ON PORT 9999 A");;

	sleep(1); //otherwise wont work
	for (;;) {

		err = write(sock, buf, strlen(buf));
		printf("Written to fd %d (result %d)\n", sock, err);
		err = read(sock, buf, sizeof(buf));
		printf("Read from fd %d (result %d)\n", sock, err);
		printf("%s\n", buf);

		buf[18]++;
		sleep(2);
	}
}


int main(int argc, char **argv)
{
  
	sleep(2);
	IP4_ADDR(&gw, 192,168,0,1);
	IP4_ADDR(&ipaddr, 0,0,0,0);
	IP4_ADDR(&netmask, 255,255,255,0);

	if (l4ankh_init())
	  return 1;

	l4_cap_idx_t c = pthread_getl4cap(pthread_self());
	cfg.send_thread = c;

	static struct option long_opts[] = {
		{"bufsize", 1, 0, BUF_SIZE },
		{"shm", 1, 0, SHM_NAME },
		GETOPT_LIST_END
	};

	while (1) {
		int optind = 0;
		int opt = getopt_long(argc, argv, "b:s:", long_opts, &optind);
		printf("getopt: %d\n", opt);

		if (opt == -1)
			break;

		switch(opt) {
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
	struct netif *n = netif_add(&my_netif,
	                            &ipaddr, &netmask, &gw,
	                            &cfg, // configuration state
	                            ankhif_init,
				    //ethernet_input
				    tcpip_input
				   );

	printf("netif_add: %p (%p)\n", n, &my_netif);
	printf("lsb hwaddr %i\n",my_netif.hwaddr[5]);
	assert(n == &my_netif);
	IP4_ADDR(&ipaddr, 192,168,0,(my_netif.hwaddr[5]%240)+3);
	netif_set_ipaddr(&my_netif,&ipaddr);

	netif_set_default(&my_netif);
	netif_set_up(&my_netif);

	while (!netif_is_up(&my_netif))
		l4_sleep(1000);
	printf("Network interface is up.\n");


	printf("IP: "); lwip_util_print_ip(&my_netif.ip_addr); printf("\n");
	printf("GW: "); lwip_util_print_ip(&my_netif.gw); printf("\n");

	//Run the two client threads for sending messages
	//on two different ports
	pthread_t thread;
	pthread_create(&thread,NULL,client,NULL);
	client2();

	l4_sleep_forever();
	return 0;
}
