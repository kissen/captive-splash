#include "error.h"

#include <ets_sys.h>
#include <gpio.h>
#include <os_type.h>
#include <osapi.h>

#include <stdint.h>

// ESP-12 modules have LED on GPIO2. Change to another GPIO
// for other boards.
static const int pin = 2;
static volatile os_timer_t main_timer;

static void set_uart0_baud_rate(uint32_t rate)
{
	// http://kacangbawang.com/esp8266-sdk-os_printf-prints-garbage
	uart_div_modify(0, UART_CLK_FREQ / rate);
}

static void tick(void *arg)
{
	static uint32_t counter;
	os_printf("counter=%d\n", counter++);
}

void ICACHE_FLASH_ATTR user_init()
{
	// init gpio subsytem & uart
	gpio_init();
	set_uart0_baud_rate(115200);

	os_printf("sizeof(int)=%d\n", sizeof(int));
	os_printf("sizeof(long)=%d\n", sizeof(long));
	os_printf("sizeof(long long)=%d\n", sizeof(long long));

	// configure wifi ap
	error_fatal("fuck");

	// setup timer (1000ms, repeating)
	os_timer_setfn((ETSTimer*) &main_timer, (os_timer_func_t *)tick, NULL);
	os_timer_arm((ETSTimer*) &main_timer, 1000, 1);
}
