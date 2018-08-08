#ifndef __CACTUSOS__SYSTEM__NETWORK__UDP_H
#define __CACTUSOS__SYSTEM__NETWORK__UDP_H

#include <common/types.h>
#include <system/network/ipv4.h>
#include <core/memorymanagement.h>

namespace CactusOS
{
    namespace system
    {
        struct UserDatagramProtocolHeader
        {
            common::uint16_t srcPort;
            common::uint16_t dstPort;
            common::uint16_t length;
            common::uint16_t checksum;
        } __attribute__((packed));

        typedef void (*SocketReceive)(common::uint8_t* packet, common::uint32_t size);

        class DHCP;
        class UserDatagramProtocolManager;

        class UDPSocket
        {
        friend class DHCP; //DHCP needs acces to the localport parameter
        friend class UserDatagramProtocolManager;
        protected:
            common::uint16_t remotePort;
            common::uint32_t remoteIP; //Target ip address
            common::uint16_t localPort;
            common::uint32_t localIP; //Our ip address

            UserDatagramProtocolManager* backend;
        public:
            bool listening;
            SocketReceive receiveHandle;
            UDPSocket(UserDatagramProtocolManager* backend);
            ~UDPSocket();

            void HandleData(common::uint8_t* data, common::uint16_t size);
            void Send(common::uint8_t* data, common::uint16_t size);
            void Disconnect();
        };


        class UserDatagramProtocolManager
        {
        protected:
            common::uint16_t freePort;

            NetworkManager* backend;
        public:
            UserDatagramProtocolManager(NetworkManager* backend);
            ~UserDatagramProtocolManager();

            void OnInternetProtocolReceived(common::uint32_t srcIP_BE, common::uint32_t dstIP_BE,
                                            common::uint8_t* payload, common::uint32_t size);

            UDPSocket* Connect(common::uint32_t ip, common::uint16_t port);
            UDPSocket* Listen(common::uint16_t port);
            void Disconnect(UDPSocket* socket);
            void Send(UDPSocket* socket, common::uint8_t* data, common::uint16_t size);
        };
    }
}

#endif
