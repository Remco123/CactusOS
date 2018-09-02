#include <common/string.h>

using namespace CactusOS;
using namespace CactusOS::common;

size_t String::strlen(const char* str)
{
    size_t len = 0;
	while (str[len])
		len++;
	return len;
}

void String::strcat(void *dest,const void *src)
{
    char * end = (char*)dest + strlen((const char*)dest);
    MemoryOperations::memcpy((char*)end,(char*)src,strlen((char*)src));
    end = end + strlen((char*)src);
    *end = '\0';
}