#ifndef CONTEXTINFO_H
#define CONTEXTINFO_H

#include <types.h>

struct ContextInfo
{
    LIBCactusOS::uint32_t virtAddrServer;
    LIBCactusOS::uint32_t virtAddrClient;
    LIBCactusOS::uint32_t bytes;
    LIBCactusOS::uint32_t width;
    LIBCactusOS::uint32_t height;
    LIBCactusOS::uint32_t x;
    LIBCactusOS::uint32_t y;
};

#endif