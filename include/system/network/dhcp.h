#ifndef __CACTUSOS__SYSTEM__NETWORK__DHCP_H
#define __CACTUSOS__SYSTEM__NETWORK__DHCP_H

#include <common/types.h>
#include <system/network/udp.h>
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
        #define DHCP_TRANSACTION_IDENTIFIER     0x55555555

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
            common::uint8_t options[64];
        } __attribute__((packed));

        enum DhcpOptions {
            DhcpOptionPadding          = 0,
            DhcpOptionSubnetMask       = 1,
            DhcpOptionRoutersOnSubnet  = 3,
            DhcpOptionDNS              = 6,
            DhcpOptionHostName         = 12,
            DhcpOptionDomainName       = 15,
            DhcpOptionRequestedIPaddr	= 50,
            DhcpOptionIPAddrLeaseTime	= 51,
            DhcpOptionOptionOverload	= 52,
            DhcpOptionMessageType		= 53,
            DhcpOptionServerIdentifier	= 54,
            DhcpOptionParamRequest	   = 55,
            DhcpOptionMsg		      	= 56,
            DhcpOptionMaxMsgSize 		= 57,
            DhcpOptionT1value	      	= 58,
            DhcpOptionT2value		      = 59,
            DhcpOptionClassIdentifier	= 60,
            DhcpOptionClientIdentifier	= 61,
            DhcpOptionEnd     		   = 255
        };

        class UDPSocket;

        class DHCP
        {
        private:
            NetworkManager* backend;

            char* getTypeString(common::uint8_t type);
            char* getNAKMessage(unsigned char* data);
        public:
            UDPSocket* dhcpSocket;

            bool Enabled;

            DHCP(NetworkManager* backend);

            void HandleUDP(unsigned char* data, unsigned int size);

            void SendDiscovery();
            void SendRequest(common::uint8_t* requestIP, common::uint8_t* serverIP);
            common::uint8_t GetDHCPMessageType(unsigned char* data);
            void ParseACK(unsigned char* data);
        };
    }
}


#endif