#include "error.h"

#include <ets_sys.h>
#include <gpio.h>
#include <os_type.h>
#include <osapi.h>

static const char *err = NULL;

void error(const char *msg)
{
	if (!err) {
		err = msg;
	}
}

void error_print(void)
{
	if (err) {
		os_printf("error: %s\n", err);
	}
}
