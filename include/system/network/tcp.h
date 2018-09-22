#ifndef __CACTUSOS__SYSTEM__NETWORK__TCP_H
#define __CACTUSOS__SYSTEM__NETWORK__TCP_H

#include <common/types.h>
#include <system/network/ipv4.h>
#include <core/memorymanagement.h>

namespace CactusOS
{
    namespace system
    {
        enum TCPSocketState
        {
            CLOSED,
            LISTEN,
            SYN_SENT,
            SYN_RECEIVED,         
            ESTABLISHED,
            FIN_WAIT1,
            FIN_WAIT2,
            CLOSING,
            TIME_WAIT,  
            CLOSE_WAIT
        };
        enum TCPFlag
        {
            FIN = 1,
            SYN = 2,
            RST = 4,
            PSH = 8,
            ACK = 16,
            URG = 32,
            ECE = 64,
            CWR = 128,
            NS = 256
        };
        struct TCPHeader
        {
            common::uint16_t srcPort;
            common::uint16_t dstPort;
            common::uint32_t sequenceNumber;
            common::uint32_t acknowledgementNumber;
            
            common::uint8_t reserved : 4;
            common::uint8_t headerSize32 : 4;
            common::uint8_t flags;
            
            common::uint16_t windowSize;
            common::uint16_t checksum;
            common::uint16_t urgentPtr;
            
            common::uint32_t options;
        } __attribute__((packed));
        struct TCPPseudoHeader
        {
            common::uint32_t srcIP;
            common::uint32_t dstIP;
            common::uint16_t protocol;
            common::uint16_t totalLength;
    } __attribute__((packed));


        class TransmissionControlProtocolManager;

        class TCPSocket
        {
        friend class TransmissionControlProtocolManager;
        protected:
            common::uint16_t remotePort;
            common::uint32_t remoteIP;
            common::uint16_t localPort;
            common::uint32_t localIP;
            common::uint32_t sequenceNumber;
            common::uint32_t acknowledgementNumber;
            
            TCPSocketState state;
            TransmissionControlProtocolManager* backend;
        public:
            SocketReceive receiveHandle;

            TCPSocket(TransmissionControlProtocolManager* backend);
            ~TCPSocket();
            bool HandleTCPData(common::uint8_t* data, common::uint32_t size);
            void Send(common::uint8_t* data, common::uint32_t size);
            void Disconnect();
        };

        class TransmissionControlProtocolManager
        {
        protected:
            TCPSocket* sockets[65535];
            common::uint16_t numSockets;
            common::uint16_t freePort;
            
            NetworkManager* backend;
        public:
            TransmissionControlProtocolManager(NetworkManager* backend);
            
            void OnInternetProtocolReceived(common::uint32_t srcIP, common::uint32_t dstIP,
                                            common::uint8_t* payload, common::uint32_t size);

            TCPSocket* Connect(common::uint32_t ip, common::uint16_t port);
            void Disconnect(TCPSocket* socket);
            void Send(TCPSocket* socket, common::uint8_t* data, common::uint32_t size, common::uint16_t flags = 0);

            TCPSocket* Listen(common::uint16_t port);
        };
    }
}

#endif