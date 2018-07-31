#ifndef __CACTUSOS__SYSTEM__NETWORK__NETWORKMANAGER_H
#define __CACTUSOS__SYSTEM__NETWORK__NETWORKMANAGER_H

#include <common/types.h>
#include <system/drivers/networkdriver.h>
#include <system/network/arp.h>
#include <system/network/ipv4.h>
#include <core/pit.h>

namespace CactusOS
{
    namespace system
    {
        #define ETHERNET_TYPE_ARP 0x0806
        #define ETHERNET_TYPE_IP  0x0800
        #define HARDWARE_TYPE_ETHERNET 0x01

        struct EtherFrameHeader
        {
            common::uint64_t dstMAC_BE : 48;
            common::uint64_t srcMAC_BE : 48;
            common::uint16_t etherType_BE;
            common::uint8_t payload[];
        } __attribute__ ((packed));

        class NetworkDriver;
        class AddressResolutionProtocol;
        class IPV4Handler;

        class NetworkManager
        {
        protected:
            NetworkDriver* netDevice;
            static NetworkManager* instance;
        public:
            AddressResolutionProtocol* arpHandler;
            IPV4Handler* ipv4Handler;
            common::uint32_t IP_BE;

            NetworkManager(NetworkDriver* device, core::PIT* pit, common::uint32_t ip_BE);
            ~NetworkManager();

            void HandlePacket(common::uint8_t* packet, common::uint32_t size);
            void SendPacket(common::uint64_t dstMAC_BE, common::uint16_t etherType_BE, common::uint8_t* buffer, common::uint32_t size);

            common::uint32_t GetIPAddress();
            common::uint64_t GetMACAddress();
        };
    }
}

#endif