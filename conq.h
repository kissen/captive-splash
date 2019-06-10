#pragma once

#include <user_interface.h>
#include <espconn.h>

/*
 * conq.h (connection queue)
 * Send TCP messages split in different buffers
 */

struct conq_part
{
	void *buf;
	size_t buflen;  // in bytes
};

/**
 * Register `partlen' many `parts' for transmission.
 */
void conq_register(struct espconn *conn, struct conq_part *parts, size_t partlen);

/**
 * Stop transmission on `conn'.
 */
void conq_unregister(struct espconn *conn);

/**
 * Start sending. Usually called in `recvcb'.
 */
void conq_start(struct espconn *conn);

/**
 * Continue sending. Usually called in `sentcb'.
 */
void conq_continue(struct espconn *conn);
