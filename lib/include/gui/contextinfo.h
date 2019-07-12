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
    LIBCactusOS::int32_t x;
    LIBCactusOS::int32_t y;
    int clientID;
};

#endif