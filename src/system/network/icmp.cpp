#include <system/network/icmp.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);

InternetControlMessageProtocol::InternetControlMessageProtocol(NetworkManager* backend)
{
    this->backend = backend;
}
InternetControlMessageProtocol::~InternetControlMessageProtocol()
{   }

void InternetControlMessageProtocol::OnInternetProtocolReceived(uint32_t srcIP_BE, uint32_t dstIP_BE,
                                uint8_t* payload, uint32_t size)
{
    InternetControlMessageProtocolMessage* msg = (InternetControlMessageProtocolMessage*)payload;
    
    switch(msg->type)
    {
        
        case 0: //Reply
            printf("ping response from "); 
            printf(Convert::IntToString(srcIP_BE & 0xFF));
            printf("."); printf(Convert::IntToString((srcIP_BE >> 8) & 0xFF));
            printf("."); printf(Convert::IntToString((srcIP_BE >> 16) & 0xFF));
            printf("."); printf(Convert::IntToString((srcIP_BE >> 24) & 0xFF));
            printf("\n");
            break;
            
        case 8: //Request
            InternetControlMessageProtocolMessage icmp;
            icmp.type = 0; // reply
            icmp.code = 0;
            icmp.data = 0x3713; // 1337
            icmp.checksum = 0;
            icmp.checksum = IPV4Handler::Checksum((uint16_t*)&icmp,
                            sizeof(InternetControlMessageProtocolMessage));
            printf("Sending ping response\n");
            backend->ipv4->Send(srcIP_BE, Convert::ByteSwap((uint16_t)0x01), (uint8_t*)&icmp, sizeof(InternetControlMessageProtocolMessage));
            break;
    }
}
void InternetControlMessageProtocol::RequestEchoReply(uint32_t ip_be)
{
    InternetControlMessageProtocolMessage icmp;
    icmp.type = 8; // ping
    icmp.code = 0;
    icmp.data = 0x3713; // 1337
    icmp.checksum = 0;
    icmp.checksum = IPV4Handler::Checksum((uint16_t*)&icmp,
        sizeof(InternetControlMessageProtocolMessage));
    
    backend->ipv4->Send(ip_be, 0x01, (uint8_t*)&icmp, sizeof(InternetControlMessageProtocolMessage));
}