#include <common/convert.h>

using namespace CactusOS::common;

char* Convert::IntToString(uint32_t n)
{
    static char ret[24];
    int numChars = 0;
    // Determine if integer is negative
    bool isNegative = false;
    if (n < 0)
    {
        n = -n;
        isNegative = true;
        numChars++;
    }
    // Count how much space we will need for the string
    int temp = n;
    do
    {
        numChars++;
        temp /= 10;
    } while (temp);

    ret[numChars] = 0;
    //Add the negative sign if needed
    if (isNegative)
        ret[0] = '-';
    // Copy digits to string in reverse order
    int i = numChars - 1;
    do
    {
        ret[i--] = n % 10 + '0';
        n /= 10;
    } while (n);
    return ret;
}

char* Convert::IntToHexString(uint8_t w)
{
    static const char* digits = "0123456789ABCDEF";
    uint32_t hexSize = sizeof(uint8_t)<<1;
    char* rc = new char[hexSize + 1]; //Terminate string with 0
    MemoryOperations::memset(rc, 0, hexSize + 1);

    for (uint32_t i=0, j=(hexSize-1)*4 ; i<hexSize; ++i,j-=4)
        rc[i] = digits[(w>>j) & 0x0f];
    return rc;
}