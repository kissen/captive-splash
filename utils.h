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

/*
 * Print buf as hexdump to serial.
 */
void utils_hexdump(void *buf, size_t buflen);

/**
 * Convert integer i in network byte order to host byte order.
 */
uint16_t htons(uint16_t host_value);

/**
 * Convert integer i in host byte order to network byte order.
 */
uint16_t ntohs(uint16_t network_value);
