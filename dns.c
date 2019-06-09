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
 * ESP8266 is little endian in the way we are using it; let's keep the structs
 * in network byte order and convert as necessary when reading/writing
 *
 * with help from https://routley.io/tech/2017/12/28/hand-writing-dns-messages.html
 */

// bitmasks for dns_header.flags
static const uint16_t QR     = 0x0080;
static const uint16_t OPCODE = 0x0078;
static const uint16_t AA     = 0x0004;
static const uint16_t TC     = 0x0002;
static const uint16_t RD     = 0x0001;
static const uint16_t RA     = 0x8000;
static const uint16_t Z      = 0x7000;
static const uint16_t RCODE  = 0x0f00;

struct dns_header
{
	uint16_t id;
	uint16_t flags;
	uint16_t qdcount;
	uint16_t ancount;
	uint16_t nscount;
	uint16_t arcount;
} __attribute__((packed));

static bool to_bool(int condition)
{
	return condition != 0;
}

static const uint8_t opcode(const struct dns_header *header)
{
	const uint16_t res =  (header->flags & OPCODE) >> 11;
	return (uint8_t) res;
}

static void print_dns_header(const struct dns_header *header)
{
	const bool qr = to_bool(header->flags & QR);
	const uint8_t op = opcode(header);
	const uint8_t tc = to_bool(header->flags & TC);
	const uint8_t rd = to_bool(header->flags & RD);

	os_printf("dns_header{id=%04x qr=%d op=%d tc=%d rd=%d qdcount=%d}",
		  header->id, qr, op, tc, rd, ntohs(header->qdcount));
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

	// (1) analyze header

	const struct dns_header *header = (struct dns_header*) pdata;
	const uint16 flags = header->flags;

	if (flags & QR || flags & OPCODE || flags & TC || flags & RD) {
		os_printf("dns: warn: unsupported flags: ");
		print_dns_header(header);
		os_printf("\n");
	}

	if (ntohs(header->qdcount) != 1) {
		os_printf("dns: warn: unsupported QDCOUNT: ");
		print_dns_header(header);
		os_printf("\n");
	}

	// (2) analyze question

	const size_t question_offset = sizeof(*header);
	os_printf("off=%d\n", question_offset);
	const char *question = &pdata[question_offset];
	int label_count = 0;

	while (*question) {
		const uint8_t label_len = (const uint8_t) *question;
		question += 1;

		char buf[label_len + 1];
		buf[label_len] = 0;

		for (size_t i = 0; i < label_len; ++i) {
			buf[i] = *question;
			question += 1;
		}

		label_count += 1;
		os_printf("label(%d)=%s\n", label_count, buf);
	}

	// (?) Send reply

	//espconn_send(conn, (uint8_t*) pdata, len);
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
