#include <common/string.h>

using namespace CactusOS;
using namespace CactusOS::common;

int String::strlen(const char* str)
{
    uint32_t len = 0;
	while (str[len])
		len++;
	return len;
}