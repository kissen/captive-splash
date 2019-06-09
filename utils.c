#include "utils.h"

uint16_t *utils_reserved(struct espconn *conn)
{
	return (uint16_t*) &conn->reverse;
}
