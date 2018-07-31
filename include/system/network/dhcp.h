#ifndef __CACTUSOS__SYSTEM__NETWORK__DHCP_H
#define __CACTUSOS__SYSTEM__NETWORK__DHCP_H

#include <common/types.h>
#include <system/network/ipv4.h>

namespace CactusOS
{
    namespace system
    {

        #define OP_REQUEST                      1
        #define OP_REPLY                        2
        #define MAGIC_COOKIE                    0x63825363
        #define OPT_PAD                         0
        #define OPT_SUBNET_MASK                 1
        #define OPT_ROUTER                      3
        #define OPT_DNS                         6
        #define OPT_REQUESTED_IP_ADDR           50
        #define OPT_LEASE_TIME                  51
        #define OPT_DHCP_MESSAGE_TYPE           53
        #define OPT_SERVER_ID                   54
        #define OPT_PARAMETER_REQUEST           55
        #define OPT_END                         255

        #define DHCP_DISCOVER                   1
        #define DHCP_OFFER                      2
        #define DHCP_REQUEST                    3
        #define DHCP_DECLINE                    4
        #define DHCP_ACK                        5
        #define DHCP_NAK                        6
        #define DHCP_RELEASE                    7
        #define DHCP_INFORM                     8


        struct DhcpHeader
        {
            common::uint8_t opcode;
            common::uint8_t htype;
            common::uint8_t hlen;
            common::uint8_t hopCount;
            common::uint32_t xid;
            common::uint16_t secCount;
            common::uint16_t flags;
            common::uint32_t clientIpAddr;
            common::uint32_t yourIpAddr;
            common::uint32_t serverIpAddr;
            common::uint32_t gatewayIpAddr;
            char clientEthAddr[6];
            common::uint8_t reserved[10];
            char serverName[64];
            char bootFilename[128];
        } __attribute__((packed));

        struct DhcpOptions
        {
            const common::uint32_t *subnetMask;
            const common::uint32_t *routerList;
            const common::uint32_t *routerEnd;
            const common::uint32_t *dnsList;
            const common::uint32_t *dnsEnd;
            const common::uint32_t *requestedIpAddr;
            common::uint32_t leaseTime;
            common::uint32_t messageType;
            const common::uint32_t *serverId;
            const common::uint8_t *parameterList;
            const common::uint8_t *parameterEnd;
        };
    }
}


#endif