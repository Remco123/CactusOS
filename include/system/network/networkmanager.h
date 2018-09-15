#ifndef __CACTUSOS__SYSTEM__NETWORKING__NETWORKMANAGER_H
#define __CACTUSOS__SYSTEM__NETWORKING__NETWORKMANAGER_H

#include <common/types.h>
#include <common/convert.h>
#include <system/drivers/networkdriver.h>
#include <core/pit.h>

#include <system/network/ethernet.h>
#include <system/network/arp.h>
#include <system/network/nettools.h>

namespace CactusOS
{
    namespace system
    {
        #define ETHERNET_TYPE_ARP 0x0806
        #define ETHERNET_TYPE_IP 0x0800
        #define MAC_BROADCAST 0xFFFFFFFFFFFF
        #define IP_BROADCAST 0xFFFFFFFF

        class NetworkDriver;
        /*///////////////
        Protocols
        ///////////////*/
        class ARPProtocol;

        class NetworkManager
        {
        protected:
            NetworkDriver* netDevice;
        public:
            ARPProtocol* arp;

            NetworkManager(NetworkDriver* net_device);

            void StartNetwork(core::PIT* pit);

            void HandleEthernetPacket(common::uint8_t* packet, common::uint32_t size);
            void SendEthernetPacket(common::uint48_t dstMAC, common::uint16_t etherType, common::uint8_t* buffer, common::uint32_t size);

            common::uint48_t GetMACAddress();
            common::uint32_t GetIPAddress();
        };
    }
}

#endif