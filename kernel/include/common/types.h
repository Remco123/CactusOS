#ifndef CACTUSOS__COMMON__TYPES_H
#define CACTUSOS__COMMON__TYPES_H

namespace CactusOS
{
    namespace common
    {
        typedef char                        int8_t;
        typedef unsigned char               uint8_t;
        typedef short                       int16_t;
        typedef unsigned short              uint16_t;
        typedef int                         int32_t;
        typedef unsigned int                uint32_t;
        typedef long long int               int64_t;
        typedef unsigned long long int      uint64_t;
        typedef unsigned long long          uintptr_t;
    }
    constexpr common::uint32_t operator"" _KB(unsigned long long no)
    {
        return no * 1024;
    }
    constexpr common::uint32_t operator"" _MB(unsigned long long no)
    {
        return no * (1024_KB);
    }
    constexpr common::uint32_t operator"" _GB(unsigned long long no)
    {
        return no * (1024_MB);
    }
}

#endif