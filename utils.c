#include "utils.h"

uint16_t ICACHE_FLASH_ATTR *utils_reserved(struct espconn *conn)
{
	return (uint16_t*) &conn->reverse;
}
