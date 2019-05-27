#include "error.h"

#include <osapi.h>

static const char *err = NULL;

void ICACHE_FLASH_ATTR error(const char *msg)
{
	if (!err) {
		err = msg;
	}
}

void ICACHE_FLASH_ATTR error_print(void)
{
	if (err) {
		os_printf("error: %s\n", err);
	}
}
