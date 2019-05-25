#include "error.h"

#include <ets_sys.h>
#include <gpio.h>
#include <os_type.h>
#include <osapi.h>

static void delay(uint32_t ms)
{
	const uint32_t target_us = system_get_time() + ms * 1000;
	while (system_get_time() < target_us);
}

void error_fatal(const char *msg)
{
	for(;;) {
		os_printf("error: %s\n", msg);
		delay(1000);
	}
}
