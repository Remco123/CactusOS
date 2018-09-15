#ifndef __CACTUSOS__SYSTEM__NETWORK__NETTOOLS_H
#define __CACTUSOS__SYSTEM__NETWORK__NETTOOLS_H

#include <common/types.h>

namespace CactusOS
{
    namespace system
    {
        class NetTools
        {
        public:
            static common::uint32_t MakeIP(common::uint8_t ip1, common::uint8_t ip2, common::uint8_t ip3, common::uint8_t ip4);
        };
    }
}

#endif