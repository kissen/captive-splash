#include "dns.h"
#include "error.h"
#include "http.h"
#include "user_config.h"

#include <ets_sys.h>
#include <gpio.h>
#include <os_type.h>
#include <osapi.h>
#include <user_interface.h>

#include <espconn.h>

#include <stdint.h>

static void ICACHE_FLASH_ATTR timer_callback(void *arg)
{
	error_print();
}

static void ICACHE_FLASH_ATTR wifi_event_callback(System_Event_t *se)
{
#if DEBUG
	switch (se->event) {
	case EVENT_STAMODE_CONNECTED:
		os_printf("EVENT_STAMODE_CONNECTED\n");
		break;
	case EVENT_STAMODE_DISCONNECTED:
		os_printf("EVENT_STAMODE_DISCONNECTED\n");
		break;
	case EVENT_STAMODE_AUTHMODE_CHANGE:
		os_printf("EVENT_STAMODE_AUTHMODE_CHANGE\n");
		break;
	case EVENT_STAMODE_GOT_IP:
		os_printf("EVENT_STAMODE_GOT_IP\n");
		break;
	case EVENT_STAMODE_DHCP_TIMEOUT:
		os_printf("EVENT_STAMODE_DHCP_TIMEOUT\n");
		break;
	case EVENT_SOFTAPMODE_STACONNECTED:
		os_printf("EVENT_SOFTAPMODE_STACONNECTED\n");
		break;
	case EVENT_SOFTAPMODE_STADISCONNECTED:
		os_printf("EVENT_SOFTAPMODE_STADISCONNECTED\n");
		break;
	case EVENT_SOFTAPMODE_PROBEREQRECVED:
		os_printf("EVENT_SOFTAPMODE_PROBEREQRECVED\n");
		break;
	case EVENT_OPMODE_CHANGED:
		os_printf("EVENT_OPMODE_CHANGED\n");
		break;
	case EVENT_MAX:
		os_printf("EVENT_MAX\n");
		break;
	}
#endif
}

void ICACHE_FLASH_ATTR user_init()
{
	// configure tty
	// http://kacangbawang.com/esp8266-sdk-os_printf-prints-garbage
	gpio_init();
	uart_div_modify(0, UART_CLK_FREQ / 115200);

	// switch to ap mode

	const uint8_t SOFT_AP_MODE = 0x02;
	if (!wifi_set_opmode_current(SOFT_AP_MODE)) {
		error("wifi_set_opmode_current");
	}

	struct softap_config ap_config = {
		.ssid = USER_CONFIG_SSID,
		.password = USER_CONFIG_PASSWORD,
		.ssid_len = 0,
		.channel = 0,
		.authmode = AUTH_OPEN,
		.ssid_hidden = 0,
		.max_connection = 4,
		.beacon_interval = 100
	};

	if (!wifi_softap_set_config_current(&ap_config)) {
		error("wifi_softap_set_config_current");
	}

	// report wifi events

	wifi_set_event_handler_cb((wifi_event_handler_cb_t) wifi_event_callback);

	// disable dhcpd while configuring ip settings

	if (!wifi_softap_dhcps_stop()) {
		error("wifi_softap_dhcps_start");
	}

	// configure our ip

	static struct ip_info ipinf;
	IP4_ADDR(&ipinf.ip, 10, 10, 10, 1);
	IP4_ADDR(&ipinf.gw, 10, 10, 10, 1);
	IP4_ADDR(&ipinf.netmask, 255, 255, 255, 0);

	if (!wifi_set_ip_info(SOFTAP_IF, &ipinf)) {
		error("wifi_set_ip_info");
	}

	// disable built-in (m)dns; it gets in the way with our fake dns(?)

	//espconn_mdns_server_unregister();
	//espconn_mdns_close();
	//espconn_mdns_disable();

	// configure & enable dhcp

	static struct dhcps_lease lease_config;
	IP4_ADDR(&lease_config.start_ip, 10, 10, 10, 10);
	IP4_ADDR(&lease_config.end_ip, 10, 10, 10, 100);

	if (!wifi_softap_set_dhcps_lease(&lease_config)) {
		error("wifi_softap_set_dhcps_lease");
	}

	uint8_t enable_router_information = 1;
	if (!wifi_softap_set_dhcps_offer_option(OFFER_ROUTER, &enable_router_information)) {
		error("wifi_softap_set_dhcps_offer_option");
	}

	for (uint8_t i = 0; i < 2; ++i) {
		espconn_dns_setserver(i, &ipinf.ip);
	}

	if (!wifi_softap_dhcps_start()) {
		error("wifi_softap_dhcps_start");
	}

	// listen to incoming requests

	if (!http_server_init()) {
		error("http_server_init");
	}

	if (!dns_server_init(&ipinf.ip)) {
		error("dns_server_init");
	}

	// set up timer

	static volatile os_timer_t main_timer;
	os_timer_setfn((ETSTimer*) &main_timer, (os_timer_func_t *)timer_callback, NULL);
	os_timer_arm((ETSTimer*) &main_timer, 1000, 1);
}
