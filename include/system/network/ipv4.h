#ifndef __CACTUSOS__SYSTEM__NETWORK__IPV4_H
#define __CACTUSOS__SYSTEM__NETWORK__IPV4_H

#include <common/types.h>
#include <system/network/networkmanager.h>

namespace CactusOS
{
    namespace system
    {
        struct InternetProtocolV4Message
        {
            common::uint8_t headerLength : 4;
            common::uint8_t version : 4;
            common::uint8_t tos;
            common::uint16_t totalLength;
            
            common::uint16_t ident;
            common::uint16_t flagsAndOffset;
            
            common::uint8_t timeToLive;
            common::uint8_t protocol;
            common::uint16_t checksum;
            
            common::uint32_t srcIP;
            common::uint32_t dstIP;
        } __attribute__((packed));

        class IPV4Handler
        {
        private:
            NetworkManager* netManager;
            common::uint32_t gatewayIP;
            common::uint32_t subnetMask;
            
        public:
            IPV4Handler(NetworkManager* backend,
                                     common::uint32_t gatewayIP, common::uint32_t subnetMask);
            ~IPV4Handler();
            
            void HandlePacket(common::uint8_t* etherframePayload, common::uint32_t size);

            void Send(common::uint32_t dstIP_BE, common::uint8_t protocol, common::uint8_t* buffer, common::uint32_t size);
            
            static common::uint16_t Checksum(common::uint16_t* data, common::uint32_t lengthInBytes);
        };
    }
}

#endif