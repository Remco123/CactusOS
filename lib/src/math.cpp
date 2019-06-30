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