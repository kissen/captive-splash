#include "error.h"
#include "http.h"
#include "utils.h"

#include <ets_sys.h>
#include <gpio.h>
#include <os_type.h>
#include <osapi.h>
#include <user_interface.h>

#include <espconn.h>

/*
 * types;
 *
 * ESP8266 is big endian so we shouldn't have to convert from network
 * byte order;
 *
 * with help from https://routley.io/tech/2017/12/28/hand-writing-dns-messages.html
 */

static const uint16_t DNS_HEADER_QR     = 0x8000;
static const uint16_t DNS_HEADER_OPCODE = 0x7800;
static const uint16_t DNS_HEADER_AA     = 0x0400;
static const uint16_t DNS_HEADER_TC     = 0x0200;
static const uint16_t DNS_HEADER_RD     = 0x0100;
static const uint16_t DNS_HEADER_RA     = 0x0080;
static const uint16_t DNS_HEADER_Z      = 0x0070;
static const uint16_t DNS_HEADER_RCODE  = 0x000f;


struct dns_header
{
	uint16_t id;
	uint16_t flags;
	uint16_t qdcount;
	uint16_t ancount;
	uint16_t nscount;
	uint16_t arcount;
} __attribute((packed))__;

static bool to_bool(int condition)
{
	return condition != 0;
}

static const uint8_t opcode(const struct dns_header *header)
{
	const uint16_t res =  (header->flags & DNS_HEADER_OPCODE) >> 11;
	return (uint8_t) res;
}

static void print_dns_header(const struct dns_header *header)
{
	const bool qr = to_bool(header->flags & DNS_HEADER_QR);
	const uint8_t op = opcode(header);
	const uint8_t tc = to_bool(header->flags & DNS_HEADER_TC);
	const uint8_t rd = to_bool(header->flags & DNS_HEADER_RD);

	os_printf("dns_header{id=%04x qr=%d op=%d tc=%d rd=%d}\n", header->id, qr, op, tc, rd);
}

/*
 * callbacks
 */

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

	const struct dns_header *header = (struct dns_header*) pdata;
	print_dns_header(header);

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

/*
 * public functions
 */

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
