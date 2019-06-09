#include "error.h"
#include "http.h"
#include "utils.h"

#include <ets_sys.h>
#include <gpio.h>
#include <os_type.h>
#include <osapi.h>
#include <user_interface.h>

#include <espconn.h>

static ICACHE_FLASH_ATTR void recvcb(void *arg, char *pdata, unsigned short len)
{
	struct espconn *conn = arg;

	os_printf("dns.c:recvb ip=%d.%d.%d.%d port=%d len=%d\n",
		conn->proto.tcp->remote_ip[0],
		conn->proto.tcp->remote_ip[1],
		conn->proto.tcp->remote_ip[2],
		conn->proto.tcp->remote_ip[3],
		conn->proto.tcp->remote_port,
		len
	);

	utils_hexdump(pdata, len);

	espconn_send(conn, (uint8_t*) pdata, len);
}

static ICACHE_FLASH_ATTR void sentcb(void *arg)
{
	struct espconn *conn = arg;

	os_printf("dns.c:sentcb ip=%d.%d.%d.%d port=%d\n",
		conn->proto.tcp->remote_ip[0],
		conn->proto.tcp->remote_ip[1],
		conn->proto.tcp->remote_ip[2],
		conn->proto.tcp->remote_ip[3],
		conn->proto.tcp->remote_port
	);
}

bool ICACHE_FLASH_ATTR dns_server_init(void)
{
	static esp_udp udp = {
		.local_port = 53,
	};

	static struct espconn udp_server = {
		.type = ESPCONN_UDP,
		.proto.udp = &udp,
	};

	if (espconn_regist_recvcb(&udp_server, recvcb) != 0) {
		error("dns: espcon_register_recvb");
		return false;
	}

	if (espconn_regist_sentcb(&udp_server, sentcb) != 0) {
		error("dns: espcon_register_sentcb");
		return false;
	}

	if (espconn_create(&udp_server) != 0) {
		error("dns: espconn_create");
		return false;
	}

	return true;
}
