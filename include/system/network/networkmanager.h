#ifndef __CACTUSOS__SYSTEM__NETWORK__NETWORKMANAGER_H
#define __CACTUSOS__SYSTEM__NETWORK__NETWORKMANAGER_H

#include <common/types.h>
#include <system/drivers/networkdriver.h>
#include <system/network/arp.h>
#include <system/network/ipv4.h>
#include <system/network/dhcp.h>
#include <system/network/icmp.h>
#include <system/network/udp.h>
#include <core/pit.h>

#include <system/console.h>

namespace CactusOS
{
    namespace system
    {
        #define ETHERNET_TYPE_ARP 0x0806
        #define ETHERNET_TYPE_IP  0x0800
        #define HARDWARE_TYPE_ETHERNET 0x01

        #define DHCP_MAX_TRIES 5

        struct EtherFrameHeader
        {
            common::uint64_t dstMAC_BE : 48;
            common::uint64_t srcMAC_BE : 48;
            common::uint16_t etherType_BE;
            common::uint8_t payload[];
        } __attribute__ ((packed));

        class NetworkDriver;
        class AddressResolutionProtocol;
        class DHCP;
        class InternetControlMessageProtocol;
        class IPV4Handler;
        class UserDatagramProtocolManager;

        class NetworkManager
        {
        friend class DHCP;
        protected:
            common::uint8_t MAC[6];
        public:
            //Variables
            NetworkDriver* netDevice;
            AddressResolutionProtocol* arp;
            DHCP* dhcp;
            InternetControlMessageProtocol* icmp;
            IPV4Handler* ipv4;
            UserDatagramProtocolManager* udp;
            bool NetworkAvailable;

            NetworkManager(NetworkDriver* device);
            ~NetworkManager();

            void StartNetwork(core::PIT* pit);

            void HandlePacket(common::uint8_t* packet, common::uint32_t size);
            void SendPacket(common::uint64_t dstMAC_BE, common::uint16_t etherType_BE, common::uint8_t* buffer, common::uint32_t size);

            common::uint32_t GetIPAddress();
            common::uint64_t GetMACAddress();
        };
    }
}

#endif