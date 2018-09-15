#ifndef __CACTUSOS__SYSTEM__NETWORK__ETHERNET_H
#define __CACTUSOS__SYSTEM__NETWORK__ETHERNET_H

#include <common/types.h>

namespace CactusOS
{
    namespace system
    {
        struct EtherFrameHeader
        {
            common::uint64_t dstMAC_BE : 48;
            common::uint64_t srcMAC_BE : 48;
            common::uint16_t etherType_BE;
        } __attribute__ ((packed));
    }
}

#endif