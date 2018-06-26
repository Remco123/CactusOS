#ifndef CACTUSOS__COMMON__MATH_H
#define CACTUSOS__COMMON__MATH_H

#include <common/types.h>

namespace CactusOS
{
    namespace common
    {
        class Math
        {
        public:
            static int Abs(int v);
            static int Sign(int v);
        };
    }
}

#endif