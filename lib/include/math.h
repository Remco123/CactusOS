#ifndef __CACTUSOSLIB__MATH_H
#define __CACTUSOSLIB__MATH_H

#include <types.h>

namespace LIBCactusOS
{
    #define MATH_PI 3.14159265358979323846

    struct MXCSR_StatusRegister
    {
        uint8_t InvalidOperationFlag : 1;
        uint8_t DenormalFlag : 1;
        uint8_t DevideByZeroFlag : 1;
        uint8_t OverflowFlag : 1;
        uint8_t UnderflowFlag : 1;
        uint8_t PrecisionFlag : 1;
        uint8_t DemormalsAreZeros : 1;
        uint8_t InvalidOperationMask : 1;
        uint8_t DenormalOperationMask : 1;
        uint8_t DevideByZeroMask : 1;
        uint8_t OverflowMask : 1;
        uint8_t UnderflowMask : 1;
        uint8_t PrecisionMask : 1;
        uint8_t RoundingControl : 2;
        uint8_t FlushToZero : 1;
        uint16_t Reserved;
    } __attribute__((packed));

    class Math
    {
    public:
        static void EnableFPU();

        static long Abs(long v);
        static double fAbs(double x);
        static long Sign(long v);
        static double sin(double x);
        static double cos(double x);

        static long Max (long a, long b);
        static long Min (long a, long b);
        static long Constrain(long x, long a, long b);
        static long Map(long x, long in_min, long in_max, long out_min, long out_max);

        static float fMod(float a, float b);
        static double floor(double x);
        static double sqrt(double n);

        static double Round(double n, uint32_t digits);
    };
}
#endif