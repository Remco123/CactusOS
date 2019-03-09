#include <convert.h>
#include <string.h>

using namespace LIBCactusOS;

int IsSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

char* Convert::IntToString(int n)
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
    memset(rc, 0, hexSize + 1);

    for (uint32_t i=0, j=(hexSize-1)*4 ; i<hexSize; ++i,j-=4)
        rc[i] = digits[(w>>j) & 0x0f];
    return rc;
}
char* Convert::IntToHexString(uint16_t w)
{
    static const char* digits = "0123456789ABCDEF";
    uint32_t hexSize = sizeof(uint16_t)<<1;
    char* rc = new char[hexSize + 1]; //Terminate string with 0
    memset(rc, 0, hexSize + 1);

    for (uint32_t i=0, j=(hexSize-1)*4 ; i<hexSize; ++i,j-=4)
        rc[i] = digits[(w>>j) & 0x0f];
    return rc;
}
char* Convert::IntToHexString(uint32_t w)
{
    static const char* digits = "0123456789ABCDEF";
    uint32_t hexSize = sizeof(uint32_t)<<1;
    char* rc = new char[hexSize + 1]; //Terminate string with 0
    memset(rc, 0, hexSize + 1);

    for (uint32_t i=0, j=(hexSize-1)*4 ; i<hexSize; ++i,j-=4)
        rc[i] = digits[(w>>j) & 0x0f];
    return rc;
}

int Convert::StringToInt(char* string)
{
    int result = 0;
    unsigned int digit;
    int sign;

    while (IsSpace(*string)) {
        string += 1;
    }

    /*
    * Check for a sign.
    */

    if (*string == '-') {
        sign = 1;
        string += 1;
    } else {
        sign = 0;
        if (*string == '+') {
            string += 1;
        }
    }

    for ( ; ; string += 1) {
        digit = *string - '0';
        if (digit > 9) {
            break;
        }
        result = (10*result) + digit;
    }

    if (sign) {
        return -result;
    }
    return result;
}