#pragma once

#include <user_interface.h>
#include <espconn.h>

#include <stdbool.h>

/*
 * conq.h (connection queue)
 * Send TCP messages split in different buffers
 *
 * HOWTO
 *
 * 1. Call `conq_create' to create a new queue.
 * 2. Register as many parts to send with `conq_register`.
 * 3. Start transmission with `conq_start'. Transmission of
 *    that part now begins.
 * 4. On the sent callback, call `conq_continue` to continue
 *    the transmission. Once all parts have been transmitted,
 *    the `conq_queue' is freed.
 *
 * Do not keep the created `conq_queue' around. If you
 * keep calling `conq_continue' like you should it will be
 * garbage collected. As such, it can be dealloacted any time.
 */

struct conq_queue;

/**
 * Initialize and allocate a new conq connection.
 * Returns NULL on error (out of memory).
 */
struct conq_queue *conq_create(struct espconn *conn);

/**
 * Register `buffer' of `len' bytes as next part.
 * Returns true on success and false on failure (bad args or out of memory).
 */
bool conq_register(struct conq_queue *queue, void *buf, size_t len);

/**
 * Start sending. Usually called in `recvcb'.
 */
void conq_start(struct espconn *conn);

/**
 * Stop transmission on `conn'.
 */
void conq_stop_and_free(struct espconn *conn);

/**
 * Continue sending. Usually called in `sentcb'.
 */
void conq_continue(struct espconn *conn);
