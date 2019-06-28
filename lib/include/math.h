#ifndef __CACTUSOSLIB__MATH_H
#define __CACTUSOSLIB__MATH_H

namespace LIBCactusOS
{
    #define MATH_PI 3.14159265358979323846

    class Math
    {
    public:
        static int Abs(int v)
        {
            if(v == 0)
                return 0;
            if(v < 0)
                return -v;
            if(v > 0)
                return v;
            return v;
        }
        static double fAbs(double x)
        {
            if(x == 0.0)
                return 0;
            if(x < 0.0)
                return -x;
            if(x > 0.0)
                return x;
            return x;
        }
        static int Sign(int v)
        {
            if(v < 0)
                return -1;
            if(v == 0)
                return 0;
            if(v > 1)
                return 1;
            return 0;
        }
        static double sin(double x)
        {
            int i = 1;
            double cur = x;
            double acc = 1;
            double fact= 1;
            double pow = x;
            while (fAbs(acc) > .00000001 &&   i < 100){
                fact *= ((2*i)*(2*i+1));
                pow *= -1 * x*x; 
                acc =  pow / fact;
                cur += acc;
                i++;
            }
            return cur;
        }
        static double cos(double x) {
            return sin(x + MATH_PI / 2.0);
        }
    };
}
#endif