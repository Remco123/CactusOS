#ifndef __CACTUSOS__SYSTEM__NETWORK__ARP_H
#define __CACTUSOS__SYSTEM__NETWORK__ARP_H

#include <common/types.h>
#include <system/network/networkmanager.h>
#include <core/pit.h>

namespace CactusOS
{
    namespace system
    {
        struct AddressResolutionProtocolMessage
        {
            common::uint16_t hardwareType;
            common::uint16_t protocol;
            common::uint8_t hardwareAddressSize; // 6
            common::uint8_t protocolAddressSize; // 4
            common::uint16_t command;
            

            common::uint64_t srcMAC : 48;
            common::uint32_t srcIP;
            common::uint64_t dstMAC : 48;
            common::uint32_t dstIP;
            
        } __attribute__((packed));

        struct ArpEntry
        {
            common::uint64_t MACAddress : 48;
            common::uint32_t IPAddress;
        } __attribute__((packed));

        class NetworkManager;

        class AddressResolutionProtocol
        {
            #define MACResolveMaxTries 10
        private:
            NetworkManager* netManager;
            core::PIT* pit;

            ArpEntry* ArpDatabase[128];
            common::uint32_t NumArpItems;
        public:
            AddressResolutionProtocol(NetworkManager* parent, core::PIT* pit);
            ~AddressResolutionProtocol();

            void HandlePacket(common::uint8_t* packet, common::uint32_t size);
            void RequestMAC(common::uint32_t IP_BE);
            common::uint64_t Resolve(common::uint32_t IP_BE);
            common::uint64_t GetMACFromCache(common::uint32_t IP_BE);
        };
    }
}

#endif