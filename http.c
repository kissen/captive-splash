#include "error.h"
#include "http.h"

#include <ets_sys.h>
#include <gpio.h>
#include <os_type.h>
#include <osapi.h>
#include <user_interface.h>

#include <espconn.h>

#define TCP_SERVER_GREETING "HALLO WELT\n"

static void ICACHE_FLASH_ATTR connectcb(void*arg)
{
    struct espconn* tcp_server_local = arg;
#if TCP_SERVER_KEEP_ALIVE_ENABLE
    espconn_set_opt(tcp_server_local, BIT(3)); //enable keep alive ,this api must call in connect callback

    uint32 keep_alvie = 0;
    keep_alvie = TCP_SERVER_KEEP_ALIVE_IDLE_S;
    espconn_set_keepalive(tcp_server_local, ESPCONN_KEEPIDLE, &keep_alvie);
    keep_alvie = TCP_SERVER_RETRY_INTVL_S;
    espconn_set_keepalive(tcp_server_local, ESPCONN_KEEPINTVL, &keep_alvie);
    keep_alvie = keep_alvie = TCP_SERVER_RETRY_INTVL_S;
    espconn_set_keepalive(tcp_server_local, ESPCONN_KEEPCNT, &keep_alvie);
    os_printf("keep alive enable\n");
#endif
    os_printf("TCP server CONNECT");
    os_printf("tcp client ip:%d.%d.%d.%d port:%d", tcp_server_local->proto.tcp->remote_ip[0],
            tcp_server_local->proto.tcp->remote_ip[1], tcp_server_local->proto.tcp->remote_ip[2],
            tcp_server_local->proto.tcp->remote_ip[3], tcp_server_local->proto.tcp->remote_port);
    espconn_send(tcp_server_local, TCP_SERVER_GREETING, strlen(TCP_SERVER_GREETING));
}

static void ICACHE_FLASH_ATTR disconcb(void* arg)
{
    struct espconn* tcp_server_local = arg;
    os_printf("TCP server DISCONNECT");
    os_printf("tcp client ip:%d.%d.%d.%d port:%d\n", tcp_server_local->proto.tcp->remote_ip[0],
            tcp_server_local->proto.tcp->remote_ip[1], tcp_server_local->proto.tcp->remote_ip[2],
            tcp_server_local->proto.tcp->remote_ip[3], tcp_server_local->proto.tcp->remote_port);
}

static void ICACHE_FLASH_ATTR sentcb(void* arg)
{
    struct espconn* tcp_server_local = arg;
    os_printf("TCP server SendCb");
    os_printf("tcp client ip:%d.%d.%d.%d port:%d\n", tcp_server_local->proto.tcp->remote_ip[0],
            tcp_server_local->proto.tcp->remote_ip[1], tcp_server_local->proto.tcp->remote_ip[2],
            tcp_server_local->proto.tcp->remote_ip[3], tcp_server_local->proto.tcp->remote_port);
}

static void ICACHE_FLASH_ATTR recvcb(void *arg, char *pdata, unsigned short len)
{
    struct espconn* tcp_server_local = arg;
    os_printf("Recv tcp client ip:%d.%d.%d.%d port:%d len:%d\n", tcp_server_local->proto.tcp->remote_ip[0],
            tcp_server_local->proto.tcp->remote_ip[1], tcp_server_local->proto.tcp->remote_ip[2],
            tcp_server_local->proto.tcp->remote_ip[3], tcp_server_local->proto.tcp->remote_port, len);
    espconn_send(tcp_server_local, pdata, len);
}

static void ICACHE_FLASH_ATTR reconcb(void *arg, sint8 err)
{
    struct espconn* tcp_server_local = arg;
    os_printf("TCP server RECONNECT");
    os_printf("status:%d\n", err);
    os_printf("tcp client ip:%d.%d.%d.%d port:%d\n", tcp_server_local->proto.tcp->remote_ip[0],
            tcp_server_local->proto.tcp->remote_ip[1], tcp_server_local->proto.tcp->remote_ip[2],
            tcp_server_local->proto.tcp->remote_ip[3], tcp_server_local->proto.tcp->remote_port\
);
}

bool ICACHE_FLASH_ATTR http_init(void)
{
	static esp_tcp tcp = {
		.local_port = 80,
	};

	static struct espconn tcp_server = {
		.type = ESPCONN_TCP,
		.proto.tcp = &tcp,
	};

	if (espconn_regist_connectcb(&tcp_server, connectcb) != 0) {
		error("espconn_regist_connectcb");
	}

	if (espconn_regist_recvcb(&tcp_server, recvcb) != 0) {
		error("espconn_regist_recvb");
	}

	if (espconn_regist_reconcb(&tcp_server, reconcb) != 0) {
		error("espconn_regist_reconcb");
	}

	if (espconn_regist_disconcb(&tcp_server, disconcb) != 0) {
		error("espconn_regist_disconcb");
	}

	if (espconn_regist_sentcb(&tcp_server, sentcb) != 0) {
		error("espconn_regist_sentcb");
	}

	if (espconn_accept(&tcp_server) != 0) {
		error("espconn_accept");
	}
}
