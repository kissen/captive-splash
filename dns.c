#include "error.h"
#include "http.h"

#include <ets_sys.h>
#include <gpio.h>
#include <os_type.h>
#include <osapi.h>
#include <user_interface.h>

#include <espconn.h>

static ICACHE_FLASH_ATTR void recvcb(void *arg, char *pdata, unsigned short len)
{
	os_printf("recvb\n");
}

static ICACHE_FLASH_ATTR void sentcb(void *arg)
{
	os_printf("sentcb\n");
}

bool ICACHE_FLASH_ATTR dns_server_init(void)
{
	static esp_udp udp = {
		.local_port = 53,
	};

	static struct espconn udp_client = {
		.type = ESPCONN_UDP,
		.proto.udp = &udp,
	};

	if (espconn_regist_recvcb(&udp_client, recvcb) != 0) {
		error("espcon_register_recvb");
		return false;
	}

	if (espconn_regist_sentcb(&udp_client, sentcb) != 0) {
		error("espcon_register_sentcb");
		return false;
	}

	if (espconn_create(&udp_client) != 0) {
		error("espconn_create");
		return false;
	}

	return true;
}
