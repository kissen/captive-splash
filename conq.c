#include "conq.h"
#include "error.h"

#include <mem.h>
#include <ets_sys.h>
#include <gpio.h>
#include <os_type.h>
#include <osapi.h>
#include <user_interface.h>

#include <espconn.h>

#define MAX_ACTIVE 8

struct conq_part
{
	// contents
	void *buf;

	// size of contents in bytes
	size_t buflen;

	// next part (if there is one)
	struct conq_part *next;
};

struct conq_queue
{
	struct espconn *conn;
	struct conq_part *parts;
	struct conq_part *current;
};

static struct conq_queue *active[MAX_ACTIVE];

struct conq_queue* ICACHE_FLASH_ATTR conq_create(struct espconn *conn)
{
	// find a place in the active array

	typedef int ssize_t;
	ssize_t active_index = -1;

	for (ssize_t i = 0; i < MAX_ACTIVE; ++i) {
		if (active[i] == NULL) {
			active_index = i;
			break;
		}
	}

	if (active_index == -1) {
		return false;
	}

	// allocate and create structure

	struct conq_queue *queue;

	if ((queue = os_zalloc(sizeof(*queue))) == NULL) {
		return NULL;
	}

	queue->conn = conn;

	// add to array and return

	active[active_index] = queue;
	return queue;
}

bool ICACHE_FLASH_ATTR conq_register(struct conq_queue *queue, void *buf, size_t len)
{
	if (queue == NULL) {
		return false;
	}

	// create part struct

	struct conq_part *part;

	if ((part = os_zalloc(sizeof(*part))) == NULL) {
		return false;
	}

	part->buf = buf;
	part->buflen = len;
	part->next = NULL;

	// add to connection if no parts were added before

	if (queue->parts == NULL) {
		queue->parts = part;
		return true;
	}

	// add to connection if this isn't the first part

	struct conq_part *ptr = queue->parts;

	while (ptr->next != NULL) {
		ptr = ptr->next;
	}

	ptr->next = part;
	return true;
}

// Return the conq_queue for an espconn or NULL
// if none could be found.
static struct conq_queue* ICACHE_FLASH_ATTR get_active(struct espconn *conn)
{
	for (size_t i = 0; i < MAX_ACTIVE; ++i) {
		struct conq_queue *stored = active[i];
		if (stored != NULL && stored->conn == conn) {
			return stored;
		}
	}

	return NULL;
}

static void ICACHE_FLASH_ATTR disconnect_task(os_event_t *e)
{
	struct espconn *conn = (struct espconn *) e->par;

	if (conn != NULL) {
		espconn_disconnect(conn);
	}
}

static void ICACHE_FLASH_ATTR disconnect(struct espconn *conn)
{
	struct conq_queue *queue;

	if ((queue = get_active(conn)) == NULL) {
		return;
	}

	static os_event_t task_queue[5];
	if (!system_os_task(disconnect_task, 2, task_queue, 5)) {
		error("system_os_task");
	}

	system_os_post(2, 0, (os_param_t) conn);
}

static ICACHE_FLASH_ATTR void ICACHE_FLASH_ATTR send_next(struct espconn *conn)
{
	// if there is no connection, do nothing

	if (conn == NULL) {
		return;
	}

	// get the associated queue
	// XXX: might break as connections probably get reused...

	struct conq_queue *queue;

	if ((queue = get_active(conn)) == NULL) {
		return;
	}

	// if there is no current part to send, we have reached the end;
	// stop the connection

	if (queue->current == NULL) {
		disconnect(queue->conn);
		return;
	}

	// ask the api to send the buffer

	struct conq_part *part = queue->current;
	espconn_send(conn, (uint8_t*) part->buf, part->buflen);

	// part sent; update current pointer to point to the
	// next part we have to send

	queue->current = queue->current->next;
}

void ICACHE_FLASH_ATTR conq_start(struct espconn *conn)
{
	// It's the first time we transmit on this queue;
	// as such initalize the current pointer to point
	// to the first part and then just transmit
	// like normal (by calling `send_next'.)

	struct conq_queue *queue;

	if ((queue = get_active(conn)) == NULL) {
		// panic?
		disconnect(queue->conn);
		return;
	}

	queue->current = queue->parts;

	send_next(conn);
}

void ICACHE_FLASH_ATTR conq_stop_and_free(struct espconn *conn)
{
	struct conq_queue *queue;

	if ((queue = get_active(conn)) == NULL) {
		return;
	}

	// delete parts

	struct conq_part *cur = queue->parts;

	while (cur != NULL) {
		struct conq_part *last = cur;
		cur = cur->next;

		os_free(last);
	}

	// remove from active array

	for (size_t i = 0; i < MAX_ACTIVE; ++i) {
		if (active[i] == queue) {
			active[i] = NULL;
			break;
		}
	}

	// delete meta data

	os_free(queue);
}

void ICACHE_FLASH_ATTR conq_continue(struct espconn *conn)
{
	send_next(conn);
}
