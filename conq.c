#include "conq.h"
#include "error.h"

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

static void ICACHE_FLASH_ATTR disconnect_task(os_event_t *e)
{
	if (current.conn == NULL) {
		return;
	}

	struct espconn *conn = current.conn;
	current.conn = NULL;

	espconn_disconnect(conn);
}

static void ICACHE_FLASH_ATTR disconnect(struct espconn *conn)
{
	static os_event_t queue[5];
	if (!system_os_task(disconnect_task, 2, queue, 5)) {
		error("system_os_task");
	}

	system_os_post(2, 0, 'a');
}

static ICACHE_FLASH_ATTR void ICACHE_FLASH_ATTR send_next(struct espconn *conn)
{
	if (conn == NULL || current.conn == NULL) {
		return;
	}

	if (current.current_part == current.part_len) {
		disconnect(conn);
		return;
	}

	if (current.current_part > current.part_len) {
		// should not happen?
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
