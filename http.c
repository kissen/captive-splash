#include "conq.h"
#include "error.h"
#include "http.h"
#include "payload.h"
#include "utils.h"

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

	conq_unregister(conn);
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

	conq_unregister(conn);
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

	conq_unregister(conn);
}

static void ICACHE_FLASH_ATTR serve_redirect(struct espconn *conn)
{
	os_printf("serve_redirect(connid=%d)\n", *utils_reserved(conn));

	static char redirect[] =
		"HTTP/1.1 307 Temporary Redirect\r\n"
		"Content-Length: 0\r\n"
		"Connection: close\r\n"
		"Location: /login-portal\r\n"
		"\r\n";

	static struct conq_part part = {
		.buf = redirect,
		.buflen = sizeof(redirect) - 1
	};

	conq_register(conn, &part, 1);
	conq_start(conn);
}

// TODO: find header
int ets_snprintf(unsigned char *str, size_t size, const char *format, ...);

static void ICACHE_FLASH_ATTR serve_html(struct espconn *conn)
{
	os_printf("serve_html(connid=%d)\n", *utils_reserved(conn));

	static struct conq_part parts[5];

	parts[0].buf = "HTTP/1.1 200 OK\r\n";
	parts[0].buflen = strlen(parts[0].buf);

	parts[1].buf = "Connection: close\r\n";
	parts[1].buflen = strlen(parts[1].buf);

	static unsigned char content_len[32];
	ets_snprintf(content_len, sizeof(content_len), "Content-Length: %d\r\n", sizeof(PAYLOAD));
	parts[2].buf = content_len;
	parts[2].buflen = strlen(parts[2].buf);

	parts[3].buf = "\r\n";
	parts[3].buflen = strlen(parts[3].buf);

	parts[4].buf = PAYLOAD;
	parts[4].buflen = sizeof(PAYLOAD);

	conq_register(conn, parts, 5);
	conq_start(conn);
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

	// (1) look for the GET

	const char *method = NULL;
	const char *path = NULL;

	for (char *line = utils_tok(pdata, len, "\r\n");
	     line;
	     line = utils_tok(NULL, 0, "\r\n"))
	{
		method = strtok(line, " \r\n");
		if (!method || strcmp(method, "GET") != 0) {
			continue;
		}

		path = strtok(NULL, " \r\n");
		if (!path) {
			continue;
		}

		os_printf("method=%s\n", method);
		os_printf("path=%s\n", path);
	}

	// (2) reply

	if (!method || !path) {
		// dunno what to do
		return;
	}

	if (!strcmp(path, "/login-portal")) {
		serve_html(conn);
	} else {
		serve_redirect(conn);
	}
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

	conq_continue(conn);
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
