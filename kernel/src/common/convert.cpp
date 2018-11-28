#include <common/convert.h>

using namespace CactusOS::common;

char* Convert::IntToString(int i)
{
    if(i == 0)
        return "0";
    static char output[24];
    char* p = &output[23];

    for(*p--=0;i;i/=10) *p--=i%10+0x30; 
    return ++p; 
}