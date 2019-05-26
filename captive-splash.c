#include "error.h"
#include "user_config.h"

#include <ets_sys.h>
#include <gpio.h>
#include <os_type.h>
#include <osapi.h>
#include <user_interface.h>

#include <espconn.h>

#include <stdint.h>

static void timer_callback(void *arg)
{
	error_print();
}

static void wifi_event_callback(System_Event_t *se)
{
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
}

static void udp_recv_callback(void *arg, char *pdata, unsigned short len)
{
	os_printf("udp_recv_callback");
}

static void udp_sent_callback(void *arg)
{
	os_printf("udp_sent_callback");
}

static void set_up_uart(void)
{
	gpio_init();

	// http://kacangbawang.com/esp8266-sdk-os_printf-prints-garbage
	uart_div_modify(0, UART_CLK_FREQ / 115200);
}

static void set_up_ap(void)
{
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

	// we will run our own dhcpd

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

	// make sure the ap sents broadcasts, not the station (if enabled)
	//const uint8_t BROADCAST_IF_AP = 2;
	//if (!wifi_set_broadcast_if(BROADCAST_IF_AP)) {
	//	error("wifi_set_broadcast_if");
	//}

	// report wifi events

	wifi_set_event_handler_cb((wifi_event_handler_cb_t) wifi_event_callback);

	// listen to packages

	static esp_udp udp = {
		.local_port = 1200,
	};

	static struct espconn udp_client = {
		.type = ESPCONN_UDP,
		.proto.udp = &udp,
	};

	if (espconn_regist_recvcb(&udp_client, udp_recv_callback) != 0) {
		error("espcon_register_recvb");
	}

	if (espconn_regist_sentcb(&udp_client, udp_sent_callback) != 0) {
		error("espcon_register_sentcb");
	}

	if (espconn_create(&udp_client) != 0) {
		error("espconn_create");
	}
}

void ICACHE_FLASH_ATTR user_init()
{
	set_up_uart();
	set_up_ap();

	// setup timer (1000ms, repeating)
	static volatile os_timer_t main_timer;
	os_timer_setfn((ETSTimer*) &main_timer, (os_timer_func_t *)timer_callback, NULL);
	os_timer_arm((ETSTimer*) &main_timer, 1000, 1);
}
