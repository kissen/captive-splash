#include "error.h"
#include "utils.h"
#include "http.h"

#include <ets_sys.h>
#include <gpio.h>
#include <os_type.h>
#include <osapi.h>
#include <user_interface.h>

#include <espconn.h>

static void ICACHE_FLASH_ATTR connectcb(void *arg)
{
	static uint16_t next_connid;

	struct espconn *conn = arg;
	*utils_reserved(conn) = next_connid++;

	os_printf("http.c:connectcb connid=%d ip=%d.%d.%d.%d port=%d\n",
		*utils_reserved(conn),
		conn->proto.tcp->remote_ip[0],
		conn->proto.tcp->remote_ip[1],
		conn->proto.tcp->remote_ip[2],
		conn->proto.tcp->remote_ip[3],
		conn->proto.tcp->remote_port
	);
}

static void ICACHE_FLASH_ATTR disconcb(void *arg)
{
	struct espconn* conn = arg;

	os_printf("http.c:disconcb connid=%d ip=%d.%d.%d.%d port=%d\n",
		*utils_reserved(conn),
		conn->proto.tcp->remote_ip[0],
		conn->proto.tcp->remote_ip[1],
		conn->proto.tcp->remote_ip[2],
		conn->proto.tcp->remote_ip[3],
		conn->proto.tcp->remote_port
	);
}

static void ICACHE_FLASH_ATTR reconcb(void *arg, sint8 err)
{
	struct espconn *conn = arg;

	os_printf("http.c:reconcb connid=%d ip=%d.%d.%d.%d port=%d status=%d\n",
		*utils_reserved(conn),
		conn->proto.tcp->remote_ip[0],
		conn->proto.tcp->remote_ip[1],
		conn->proto.tcp->remote_ip[2],
		conn->proto.tcp->remote_ip[3],
		conn->proto.tcp->remote_port,
		err
	);
}

static void ICACHE_FLASH_ATTR recvcb(void *arg, char *pdata, unsigned short len)
{
	struct espconn *conn = arg;

	os_printf("http.c:recvb connid=%d ip=%d.%d.%d.%d port=%d len=%d\n",
		*utils_reserved(conn),
		conn->proto.tcp->remote_ip[0],
		conn->proto.tcp->remote_ip[1],
		conn->proto.tcp->remote_ip[2],
		conn->proto.tcp->remote_ip[3],
		conn->proto.tcp->remote_port,
		len
	);

#if DEBUG
	utils_hexdump(pdata, len);
#endif
}

static void ICACHE_FLASH_ATTR sentcb(void *arg)
{
	struct espconn *conn = arg;

	os_printf("http.c:sentcb connid=%d ip=%d.%d.%d.%d port=%d\n",
		*utils_reserved(conn),
		conn->proto.tcp->remote_ip[0],
		conn->proto.tcp->remote_ip[1],
		conn->proto.tcp->remote_ip[2],
		conn->proto.tcp->remote_ip[3],
		conn->proto.tcp->remote_port
	);
}

bool ICACHE_FLASH_ATTR http_server_init(void)
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
		return false;
	}

	if (espconn_regist_disconcb(&tcp_server, disconcb) != 0) {
		error("espconn_regist_disconcb");
		return false;
	}

	if (espconn_regist_reconcb(&tcp_server, reconcb) != 0) {
		error("espconn_regist_reconcb");
		return false;
	}

	if (espconn_regist_recvcb(&tcp_server, recvcb) != 0) {
		error("espconn_regist_recvb");
		return false;
	}

	if (espconn_regist_sentcb(&tcp_server, sentcb) != 0) {
		error("espconn_regist_sentcb");
		return false;
	}

	if (espconn_accept(&tcp_server) != 0) {
		error("espconn_accept");
		return false;
	}

	return true;
}
