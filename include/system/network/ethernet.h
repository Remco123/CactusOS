#ifndef __CACTUSOS__SYSTEM__NETWORK__ETHERNET_H
#define __CACTUSOS__SYSTEM__NETWORK__ETHERNET_H

#include <common/types.h>

namespace CactusOS
{
    namespace system
    {
        //Types
        #define ETHERNET_TYPE_ARP 0x0806
        #define ETHERNET_TYPE_IP  0x0800

        #define HARDWARE_TYPE_ETHERNET 0x01

        struct EtherFrameHeader
        {
            common::uint64_t dstMAC_BE;
            common::uint64_t srcMAC_BE;
            common::uint16_t etherType_BE;
            common::uint8_t payload[];
        } __attribute__ ((packed));
    }
}

#endif