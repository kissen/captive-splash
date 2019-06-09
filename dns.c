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
 * globals
 */

// the ip addr we always use for a reply
static ip_addr_t dns_reply_with;

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

struct dns_answer
{
	uint16_t name;
	uint16_t type;
	uint16_t class;
	uint32_t ttl;
	uint16_t rdlength;
	uint32_t rdata;
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

	os_printf("dns_header{id=%04x qr=%d op=%d tc=%d rd=%d qdcount=%d}\n",
		  header->id, qr, op, tc, rd, ntohs(header->qdcount));
}

/*
 * callbacks
 */

static ICACHE_FLASH_ATTR void recvcb(void *arg, char *pdata, unsigned short len)
{
	struct espconn *conn = arg;

	remot_info *remote = NULL;
	espconn_get_connection_info(conn, &remote, 0);

	os_printf("dns.c:recvb connid=%04x local_ip=%d.%d.%d.%d local_port=%d remote_ip=%d.%d.%d.%d remote_port=%d len=%d\n",
		*utils_reserved(conn),
		conn->proto.udp->local_ip[0],
		conn->proto.udp->local_ip[1],
		conn->proto.udp->local_ip[2],
		conn->proto.udp->local_ip[3],
		conn->proto.udp->local_port,
		remote->remote_ip[0],
		remote->remote_ip[1],
		remote->remote_ip[2],
		remote->remote_ip[3],
		remote->remote_port,
		len
	);

#if DEBUG
	utils_hexdump(pdata, len);
#endif

	// (1) analyze header

	const struct dns_header *header = (struct dns_header*) pdata;
#if DEBUG
	print_dns_header(header);
#endif

	// (2) analyze question

	const size_t question_offset = sizeof(*header);
	const char *question = &pdata[question_offset];

	size_t question_len = 0;
	for (const char *ptr = question; *ptr; ++ptr) {
		question_len += 1;
	}

	question_len += 1;  // terminating zero
	question_len += 2;  // qtype
	question_len += 2;  // qclass

	// (3) construct reply

	struct dns_header reply_header = {
		.id = header->id,
		.flags = QR | AA | RD,
		.qdcount = header->qdcount,
		.ancount = htons(1),
		.nscount = 0,
		.arcount = 0
	};

	uint16_t qtype, qclass;
	memcpy(&qtype, &question[question_len - 4], 2);
	memcpy(&qclass, &question[question_len - 2], 2);

	struct dns_answer answer = {
		.name = 0x0cc0,
		.type = qtype,
		.class = qclass,
		.ttl = htonl(5 * 60),
		.rdlength = htons(4),
		.rdata = dns_reply_with.addr  // XXX: endianess???
	};

	// (4) send reply

	static uint8_t sendbuf[256];

	const size_t total_size = sizeof(reply_header) + question_len + sizeof(answer);

	if (total_size > sizeof(sendbuf)) {
		os_printf("dns: would have to generate %d bytes, that's too much", total_size);
		return;
	}

	size_t sendoff = 0;
	memcpy(&sendbuf[sendoff], &reply_header, sizeof(reply_header));
	sendoff += sizeof(reply_header);
	memcpy(&sendbuf[sendoff], question, question_len);
	sendoff += question_len;
	memcpy(&sendbuf[sendoff], &answer, sizeof(answer));
	sendoff += sizeof(answer);

#if DEBUG
	utils_hexdump(sendbuf, total_size);
#endif

	memcpy(&conn->proto.udp->remote_ip, &remote->remote_ip, sizeof(remote->remote_ip));
	memcpy(&conn->proto.udp->remote_port, &remote->remote_port, sizeof(remote->remote_port));

	int8_t err;
	if ((err = espconn_send(conn, (uint8_t*) sendbuf, total_size)) != 0) {
		os_printf("espconn_send failed; err=%d\n", err);
	};
}

static ICACHE_FLASH_ATTR void sentcb(void *arg)
{
	struct espconn *conn = arg;

	os_printf("dns.c:sentcb connid=%04x local_ip=%d.%d.%d.%d local_port=%d remote_ip=%d.%d.%d.%d remote_port=%d\n",
		*utils_reserved(conn),
		conn->proto.udp->local_ip[0],
		conn->proto.udp->local_ip[1],
		conn->proto.udp->local_ip[2],
		conn->proto.udp->local_ip[3],
		conn->proto.udp->local_port,
		conn->proto.udp->remote_ip[0],
		conn->proto.udp->remote_ip[1],
		conn->proto.udp->remote_ip[2],
		conn->proto.udp->remote_ip[3],
		conn->proto.udp->remote_port
	);

}

/*
 * public functions
 */

bool ICACHE_FLASH_ATTR dns_server_init(ip_addr_t *http_addr)
{
	dns_reply_with = *http_addr;

	static esp_udp server_udp = {
		.local_port = 53,
	};

	static struct espconn server_udp_conn = {
		.type = ESPCONN_UDP,
		.proto.udp = &server_udp,
	};

	if (espconn_regist_recvcb(&server_udp_conn, recvcb) != 0) {
		error("dns: espcon_register_recvb");
		return false;
	}

	if (espconn_regist_sentcb(&server_udp_conn, sentcb) != 0) {
		error("dns: espcon_register_sentcb");
		return false;
	}

	if (espconn_create(&server_udp_conn) != 0) {
		error("dns: espconn_create");
		return false;
	}

	*utils_reserved(&server_udp_conn) = 0x5555;

	return true;
}
