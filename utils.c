#include "utils.h"

#include <osapi.h>

uint16_t ICACHE_FLASH_ATTR *utils_reserved(struct espconn *conn)
{
	return (uint16_t*) &conn->reverse;
}

static bool ICACHE_FLASH_ATTR hexdump_is_printable(const unsigned char c)
{
	return c >= ' ' && c <= '~';
}

// dump ascii version of str[off] for len bytes
static void ICACHE_FLASH_ATTR hexdump_str(const unsigned char *str, size_t off, size_t len)
{
	for (size_t i = off; i < off + len; ++i) {
		if (hexdump_is_printable(str[i])) {
			os_printf("%c", str[i]);
		} else {
			os_printf(".");
		}
	}
}

void ICACHE_FLASH_ATTR utils_hexdump(void *buf, size_t buflen)
{
	const uint8_t *data = buf;

	for (size_t i = 0; i < buflen; ++i) {
		// we print rows of 8 bytes each
		const size_t col = i % 8;

		if (col == 0) {
			os_printf("%04x    ", i);
		}

		// print hex byte
		os_printf("%02x", data[i]);

		switch (col) {
		case 3:
			os_printf("    ");
			break;
		case 7:
		{
			os_printf("    |");
			hexdump_str(data, i - 7, 8);
			os_printf("|\n");
			break;
		}
		default:
			os_printf("  ");
		}
	}

	os_printf("\n");
}
