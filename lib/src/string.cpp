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

int strcmp(const char *s1, const char *s2)
{
    while ((*s1 == *s2) && *s1) { ++s1; ++s2; }
    return (((int) (unsigned char) *s1) - ((int) (unsigned char) *s2) == 0) ? 1 : 0;
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
List<char*> str_Split(const char* str, char d)
{
    List<char*> Result;

    int amountOfDelims = 0;
    while(str_IndexOf(str, d, amountOfDelims) != -1)
        amountOfDelims++;
    
    if(amountOfDelims == 0)
        return Result;
    
    int* delimOffsets = new int[amountOfDelims];
    for(int i = 0; i < amountOfDelims; i++)
        delimOffsets[i] = str_IndexOf(str, d, i);

    for(int i = 0; i < amountOfDelims; i++)
    {
        int len = i >= 1 ? (delimOffsets[i] - delimOffsets[i - 1] - 1) : delimOffsets[i];

        char* partStr = new char[len + 1];
        memcpy(partStr, str + (i >= 1 ? delimOffsets[i - 1] + 1 : 0), len);
        partStr[len] = '\0';

        Result.push_back(partStr);
    }

    //Don't forget to add the remaining part of the string
    int stringRemainder = strlen(str) - delimOffsets[amountOfDelims - 1];
    char* lastStr = new char[stringRemainder];
    memcpy(lastStr, str + delimOffsets[amountOfDelims - 1] + 1, stringRemainder);
    lastStr[stringRemainder] = '\0';

    Result.push_back(lastStr);

    return Result;
}
char* str_Add(char* str, char c)
{
	int oldLen = strlen(str);
    char* newStr = new char[oldLen + 1];
    memcpy(newStr, str, oldLen);
    newStr[oldLen] = c;
    newStr[oldLen + 1] = '\0';
	
	delete str;
	return newStr;
}
char* str_Combine(char* part1, char* part2)
{
    int len1 = strlen(part1);
    int len2 = strlen(part2);

    char* res = new char[len1 + len2 + 1];
    memcpy(res, part1, len1);
    memcpy(res + len1, part2, len2);
    res[len1 + len2] = '\0';
    return res;
}
bool isvalid(unsigned char key)
{
    return ((int)key > 31 && (int)key < 127) || key == '\n' || key == '\b' || key == '\t';
}