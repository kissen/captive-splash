#include "conq.h"

#include <ets_sys.h>
#include <gpio.h>
#include <os_type.h>
#include <osapi.h>
#include <user_interface.h>

#include <espconn.h>

struct conq_connection
{
	struct espconn *conn;
	struct conq_part *parts;
	size_t current_part;
	size_t part_len;
};

// XXX: only one supported
static struct conq_connection current;

void ICACHE_FLASH_ATTR conq_register(struct espconn *conn, struct conq_part *parts, size_t partlen)
{
	current.conn = conn;
	current.parts = parts;
	current.current_part = 0;
	current.part_len = partlen;
}

void ICACHE_FLASH_ATTR conq_unregister(struct espconn *conn)
{
	if (current.conn == conn) {
		current.conn = NULL;
	}
}

static void ICACHE_FLASH_ATTR send_next(struct espconn *conn)
{
	if (conn == NULL || conn != current.conn) {
		return;
	}

	if (current.current_part >= current.part_len) {
		return;
	}

	struct conq_part *part = &current.parts[current.current_part];
	current.current_part += 1;

	espconn_send(conn, (uint8_t*) part->buf, part->buflen);
}

void ICACHE_FLASH_ATTR conq_start(struct espconn *conn)
{
	send_next(conn);
}

void ICACHE_FLASH_ATTR conq_continue(struct espconn *conn)
{
	send_next(conn);
}
