#include <common/string.h>

using namespace CactusOS;
using namespace CactusOS::common;

int String::strlen(const char* str)
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

int String::Split(const char *str, char c, char*** arr)
{
    int count = 1;
    int token_len = 1;
    int i = 0;
    char *p;
    char *t;

    p = (char*)str;
    while (*p != '\0')
    {
        if (*p == c)
            count++;
        p++;
    }

    *arr = (char**) core::MemoryManager::activeMemoryManager->malloc(sizeof(char*) * count);
    if (*arr == NULL)
        return 0;

    p = (char*)str;
    while (*p != '\0')
    {
        if (*p == c)
        {
            (*arr)[i] = (char*)  core::MemoryManager::activeMemoryManager->malloc( sizeof(char) * token_len );
            if ((*arr)[i] == NULL)
                return 0;

            token_len = 0;
            i++;
        }
        p++;
        token_len++;
    }
    (*arr)[i] = (char*)  core::MemoryManager::activeMemoryManager->malloc( sizeof(char) * token_len );
    if ((*arr)[i] == NULL)
        return 0;

    i = 0;
    p = (char*)str;
    t = ((*arr)[i]);
    while (*p != '\0')
    {
        if (*p != c && *p != '\0')
        {
            *t = *p;
            t++;
        }
        else
        {
            *t = '\0';
            i++;
            t = ((*arr)[i]);
        }
        p++;
    }

    return count;
}

bool String::Contains(const char* str, char c)
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