#include <common/math.h>

using namespace CactusOS::common;

int Math::Abs(int v)
{
    if(v == 0)
        return 0;
    if(v < 0)
        return -v;
    if(v > 0)
        return v;
    return v;
}
int Math::Sign(int v)
{
    if(v < 0)
        return -1;
    if(v == 0)
        return 0;
    if(v > 1)
        return 1;
    return 0;
}