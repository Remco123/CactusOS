#include <system/network/nettools.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;

uint32_t NetTools::MakeIP(common::uint8_t ip1, common::uint8_t ip2, common::uint8_t ip3, common::uint8_t ip4)
{
    return ((uint32_t)ip1 << 24)
        |  ((uint32_t)ip2 << 16)
        |  ((uint32_t)ip3 << 8)
        | (uint32_t)ip4;
}