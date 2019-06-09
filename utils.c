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
	bool printed_newline = false;

	for (size_t i = 0; i < buflen; ++i) {
		printed_newline = false;

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
			printed_newline = true;
			break;
		}
		default:
			os_printf("  ");
		}
	}

	if (!printed_newline) {
		os_printf("\n");
	}
}

static ICACHE_FLASH_ATTR uint16_t swap_bytes16(uint16_t i)
{
	return (i >> 8 & 0xff) | (i << 8);
}

uint16_t ICACHE_FLASH_ATTR htons(uint16_t i)
{
	return swap_bytes16(i);
}

uint16_t ICACHE_FLASH_ATTR ntohs(uint16_t i)
{
	return swap_bytes16(i);
}

static ICACHE_FLASH_ATTR uint32_t swap_bytes32(uint32_t i)
{
	return (i >> 24) | ((i & 0xff0000) >> 8) | ((i & 0xff00) << 8) | (i << 24);

}

uint32_t ICACHE_FLASH_ATTR htonl(uint32_t i)
{
	return swap_bytes32(i);
}

uint32_t ICACHE_FLASH_ATTR ntohl(uint32_t i)
{
	return swap_bytes32(i);
}
