#pragma once

#include <stdbool.h>

/*
 * http.h
 * Define HTTP logic for captive-splash.
 */

/*
 * Initialize the HTTP server. Return true on success and false on
 * failure.
 */
bool http_server_init(void);
