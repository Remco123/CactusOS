#include <string.h>

void* memmove(void* dstptr, const void* srcptr, size_t size) {
	unsigned char* dst = (unsigned char*) dstptr;
	const unsigned char* src = (const unsigned char*) srcptr;
	if (dst < src) {
		for (size_t i = 0; i < size; i++)
			dst[i] = src[i];
	} else {
		for (size_t i = size; i != 0; i--)
			dst[i-1] = src[i-1];
	}
	return dstptr;
}

size_t strlen(const char* str) {
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

int memcmp(const void* aptr, const void* bptr, size_t size) {
	const unsigned char* a = (const unsigned char*) aptr;
	const unsigned char* b = (const unsigned char*) bptr;
	for (size_t i = 0; i < size; i++) {
		if (a[i] < b[i])
			return -1;
		else if (b[i] < a[i])
			return 1;
	}
	return 0;
}

void* memset(void* bufptr, int value, size_t size) {
	unsigned char* buf = (unsigned char*) bufptr;
	for (size_t i = 0; i < size; i++)
		buf[i] = (unsigned char) value;
	return bufptr;
}

void* memcpy(void* __restrict__ dstptr, const void* __restrict__ srcptr, size_t size) {
	unsigned char* dst = (unsigned char*) dstptr;
	const unsigned char* src = (const unsigned char*) srcptr;
	for (size_t i = 0; i < size; i++)
		dst[i] = src[i];
	return dstptr;
}

int str_IndexOf(const char* str, char c, int skip)
{
    int hits = 0;
    int i = 0;
    while(str[i])
    {
        if(str[i] == c && hits++ == skip)
            return i;
        i++;
    }
    return -1;
}

bool str_Contains(const char* str, char c)
{
    int i = 0;
	while (str[i])
    {
        if(str[i] == c)
            return true;
		i++;
    }
    return false;
}

char* str_Uppercase(char* str)
{ 
    int len = strlen(str);
    int i = 0;
	while (i < len)
	{
		if ((short)str[i] >= 97 && (short)str[i] <= 122)
			str[i] -= 32;
		i++;
	}
    return str;
}
char* str_Lowercase(char* str)
{
    int len = strlen(str);
    int i = 0;
	while (i < len)
	{
		if ((short)str[i] >= 65 && (short)str[i] <= 90)
			str[i] += 32;
		i++;
	}
    return str;
}