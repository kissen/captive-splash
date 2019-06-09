#pragma once

#include <stdbool.h>

/*
 * dns.h
 * Define DNS logic for captive-splash.
 */

/*
 * Initialize the DNS server. Return true on success and false on
 * failure.
 */
bool dns_server_init(void);
