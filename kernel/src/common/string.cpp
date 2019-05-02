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

int String::IndexOf(const char* str, char c, uint32_t skip)
{
    uint32_t hits = 0;
    int i = 0;
    while(str[i])
    {
        if(str[i] == c && hits++ == skip)
            return i;
        i++;
    }
    return -1;
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

List<char*> String::Split(const char* str, char d)
{
    List<char*>* Result = new List<char*>();

    int amountOfDelims = 0;
    while(String::IndexOf(str, d, amountOfDelims) != -1)
        amountOfDelims++;
    
    if(amountOfDelims == 0)
        return *Result;
    
    int* delimOffsets = new int[amountOfDelims];
    for(int i = 0; i < amountOfDelims; i++)
        delimOffsets[i] = String::IndexOf(str, d, i);

    for(int i = 0; i < amountOfDelims; i++)
    {
        int len = i >= 1 ? (delimOffsets[i] - delimOffsets[i - 1] - 1) : delimOffsets[i];

        char* partStr = new char[len + 1];
        MemoryOperations::memcpy(partStr, str + (i >= 1 ? delimOffsets[i - 1] + 1 : 0), len);
        partStr[len] = '\0';

        Result->push_back(partStr);
    }

    //Don't forget to add the remaining part of the string
    int stringRemainder = String::strlen(str) - delimOffsets[amountOfDelims - 1];
    char* lastStr = new char[stringRemainder];
    MemoryOperations::memcpy(lastStr, str + delimOffsets[amountOfDelims - 1] + 1, stringRemainder);
    lastStr[stringRemainder] = '\0';

    Result->push_back(lastStr);

    return *Result;
}

char* String::Uppercase(char* str)
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
char* String::Lowercase(char* str)
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

char* String::strcpy(char *s1, const char *s2)
{
	strncpy(s1, s2, strlen(s2) + 1);
	s1[strlen(s2)] = '\0'; //tack on the null terminating character if it wasn't already done
	return s1;
}

char* String::strncpy(char *s1, const char *s2, int n)
{
	unsigned int extern_iter = 0;

	unsigned int iterator = 0;
	for (iterator = 0; iterator < n; iterator++) //iterate through s2 up to char n, copying them to s1
	{
		if (s2[iterator] != '\0')
			s1[iterator] = s2[iterator];
		else //the end of s2 was found prematurely - copy the null character, update external iterator and quit for loop
		{
			s1[iterator] = s2[iterator];
			extern_iter = iterator + 1;
			break;
		}
	}

	while (extern_iter < n) //while there are still spaces that need to be filled with null characters, fill them
	{
		s1[extern_iter] = '\0';
		extern_iter++;
	}

	return s1;
}