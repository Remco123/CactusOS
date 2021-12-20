#include <math.h>
#include <types.h>
#include <log.h>

using namespace LIBCactusOS;

uint32_t getMXCSR (void)
{
  uint32_t temp;
  asm volatile("stmxcsr %0" : "=m" (temp));
  return temp;
}

void setMXCSR (uint32_t val)
{
  asm volatile ("ldmxcsr %0" : : "m" (val));
}

void Math::EnableFPU()
{
    asm volatile("fninit"); //First initialize the fpu and set to default state

    uint32_t v = getMXCSR();

    MXCSR_StatusRegister* mxcsr = (MXCSR_StatusRegister*)&v;

    mxcsr->FlushToZero = 1;
    mxcsr->PrecisionMask = 1;

    setMXCSR(v);
}

long Math::Abs(long v)
{
    if(v == 0)
        return 0;
    if(v < 0)
        return -v;
    if(v > 0)
        return v;
    return v;
}
double Math::fAbs(double x)
{
    if(x == 0.0)
        return 0;
    if(x < 0.0)
        return -x;
    if(x > 0.0)
        return x;
    return x;
}
long Math::Sign(long v)
{
    if(v < 0)
        return -1;
    if(v == 0)
        return 0;
    if(v > 1)
        return 1;
    return 0;
}
double Math::sin(double x)
{
    int i = 1;
    double cur = x;
    double acc = 1;
    double fact= 1;
    double pow = x;
    while (fAbs(acc) > .00000001 && i < 100){
        fact *= ((2*i)*(2*i+1));
        pow *= -1 * x*x; 
        acc =  pow / fact;
        cur += acc;
        i++;
    }
    return cur;
}
double Math::cos(double x) {
    return sin(x + MATH_PI / 2.0);
}
long Math::Max (long a, long b) {
  return (a < b) ? b : a;
}
long Math::Min(long a, long b) {
  return !(b < a) ? a : b;
}
long Math::Constrain(long x, long a, long b)
{
    if(x < a)
        return a;
    else if(b < x)
        return b;
    else
        return x;
}
long Math::Map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
float Math::fMod(float a, float b)
{
    return (a - b * floor(a / b));
}
double Math::floor(double x) {
    if (x >= __LONG_LONG_MAX__ || x <= (-__LONG_LONG_MAX__-1) || x != x) {
        /* handle large values, infinities and nan */
        return x;
    }
    long long n = (long long)x;
    double d = (double)n;
    if (d == x || x >= 0)
        return d;
    else
        return d - 1;
}
double Math::sqrt(double n)
{
    double lo = 0, hi = n, mid;
    for(int i = 0 ; i < 1000 ; i++){
        mid = (lo+hi)/2;
        if(mid*mid == n) return mid;
        if(mid*mid > n) hi = mid;
        else lo = mid;
    }
    return mid;
}
double Math::Round(double x, uint32_t digits)
{
    if (digits > 0) {
        return Round(x*10.0, digits-1)/10.0;
    }
    else {
        return (double)(int)(x);
    }
}