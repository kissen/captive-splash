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

/*
 * Like strtok(3), except `str' is limited to `len' bytes, i.e. it
 * does not have to be zero-terminated.
 *
 * On first call, set `str' to the string you wish to tokenize,
 * on calls afterwards set `str' to NULL. `len' is only evaluated
 * if `str != NULL'.
 *
 * The functions returns the token as zero-terminated string or NULL
 * if no more tokens were found. Note: If the last character of of
 * `str' is not in `delim' and not zero, the last token will not be
 * returned.
 */
char *utils_tok(char *str, size_t len, const char *delim);

/**
 * Convert integer i in network byte order to host byte order.
 */
uint16_t htons(uint16_t host_value);

/**
 * Convert integer i in host byte order to network byte order.
 */
uint16_t ntohs(uint16_t network_value);


/**
 * Convert integer i in network byte order to host byte order.
 */
uint32_t htonl(uint32_t host_value);

/**
 * Convert integer i in host byte order to network byte order.
 */
uint32_t ntohl(uint32_t network_value);
