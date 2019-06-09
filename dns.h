#pragma once

#include <stdbool.h>

#include <user_interface.h>
#include <espconn.h>

/*
 * dns.h
 * Define DNS logic for captive-splash.
 */

/*
 * Initialize the DNS server. Return true on success and false on
 * failure.
 */
bool dns_server_init(ip_addr_t *http_addr);
