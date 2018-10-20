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

bool String::strncmp(const char* s1, const char* s2, int n)
{
    while(n--)
        if(*s1++ != *s2++)
            return false;
    return true;
}

void String::split(const char *original, const char *delimiter, char ** & buffer, int & numStrings, int * & stringLengths)
{
    const int lo = strlen(original);
    const int ld = strlen(delimiter);
    if(ld > lo){
        buffer = 0;
        numStrings = 0;
        stringLengths = 0;
        return;
    }

    numStrings = 1;

    for(int i = 0;i < (lo - ld);i++){
        if(strncmp(&original[i], delimiter, ld) == true) {
            i += (ld - 1);
            numStrings++;
        }
    }

    stringLengths = (int *) core::MemoryManager::activeMemoryManager->malloc(sizeof(int) * numStrings);

    int currentStringLength = 0;
    int currentStringNumber = 0;
    int delimiterTokenDecrementCounter = 0;
    for(int i = 0;i < lo;i++){
        if(delimiterTokenDecrementCounter > 0){
            delimiterTokenDecrementCounter--;
        } else if(i < (lo - ld)){
            if(strncmp(&original[i], delimiter, ld) == true){
                stringLengths[currentStringNumber] = currentStringLength;
                currentStringNumber++;
                currentStringLength = 0;
                delimiterTokenDecrementCounter = ld - 1;
            } else {
                currentStringLength++;
            }
        } else {
            currentStringLength++;
        }

        if(i == (lo - 1)){
            stringLengths[currentStringNumber] = currentStringLength;
        }
    }

    buffer = (char **) core::MemoryManager::activeMemoryManager->malloc(sizeof(char *) * numStrings);
    for(int i = 0;i < numStrings;i++){
        buffer[i] = (char *) core::MemoryManager::activeMemoryManager->malloc(sizeof(char) * (stringLengths[i] + 1));
    }

    currentStringNumber = 0;
    currentStringLength = 0;
    delimiterTokenDecrementCounter = 0;
    for(int i = 0;i < lo;i++){
        if(delimiterTokenDecrementCounter > 0){
            delimiterTokenDecrementCounter--;
        } else if(currentStringLength >= stringLengths[currentStringNumber]){
            buffer[currentStringNumber][currentStringLength] = 0;
            delimiterTokenDecrementCounter = ld - 1;
            currentStringLength = 0;
            currentStringNumber++;
        } else {
            buffer[currentStringNumber][currentStringLength] = (char)original[i];
            currentStringLength++;
        }
    }
    buffer[currentStringNumber][currentStringLength] = 0;
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