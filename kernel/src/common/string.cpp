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

bool String::strcmp(const char* strA, const char* strB)
{
    while(*strA == *strB)
    {
        if(*strA == '\0')
            return true; //If we reach this the strings are equal
        ++strA;
        ++strB;
    }
    return false;
}

bool String::strncmp(const char* s1, const char* s2, int n)
{
    while(n--)
        if(*s1++ != *s2++)
            return false;
    return true;
}