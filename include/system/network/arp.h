#ifndef __CACTUSOS__SYSTEM__NETWORK__ARP_H
#define __CACTUSOS__SYSTEM__NETWORK__ARP_H

#include <system/network/networkmanager.h>

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

        class ARPProtocol
        {
        private:
            NetworkManager* netManager;
            core::PIT* pit;

            ArpEntry* ArpDatabase[128];
            common::uint32_t NumArpItems;

            common::uint48_t GetMACFromDatabase(common::uint32_t ip);
        public:
            ARPProtocol(NetworkManager* parent, core::PIT* pit);

            void HandlePacket(common::uint8_t* packet, common::uint32_t size);
            void SendRequest(common::uint32_t ip);
            void SendResponse(common::uint48_t TargetMAC, common::uint32_t TargetIP);
            common::uint48_t Resolve(common::uint32_t ip);

            void PrintArpEntries();
        };
    }
}

#endif