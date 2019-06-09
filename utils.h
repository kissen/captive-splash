#pragma once

#include <user_interface.h>
#include <espconn.h>

#include <stdint.h>

/*
 * utils.h
 * Util functions used in the project.
 */

/*
 * Return a pointer to the reserved area in an espconn.
 * We can use it store all kinds of fun information.
 */
uint16_t *utils_reserved(struct espconn *conn);
