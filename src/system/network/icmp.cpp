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

void InternetControlMessageProtocol::OnInternetProtocolReceived(uint32_t srcIP, uint32_t dstIP,
                                uint8_t* payload, uint32_t size)
{
    printf("Received ICMP\n");
    InternetControlMessageProtocolMessage* msg = (InternetControlMessageProtocolMessage*)payload;
    
    switch(msg->type)
    {
        
        case 0: //Reply
            printf("ping response from "); 
            NetTools::PrintIP(srcIP);
            printf("\n");
            break;
            
        case 8: //Request
            InternetControlMessageProtocolMessage icmp;
            icmp.type = 0; // reply
            icmp.code = 0;
            icmp.data = msg->data;
            icmp.checksum = 0;
            icmp.checksum = IPV4Protocol::Checksum((uint16_t*)&icmp,
                            sizeof(InternetControlMessageProtocolMessage));
            printf("Sending ping response to ip: "); NetTools::PrintIP(srcIP); printf("\n");
            backend->ipv4->Send(srcIP, 0x01, (uint8_t*)&icmp, sizeof(InternetControlMessageProtocolMessage));
            break;
    }
}
void InternetControlMessageProtocol::RequestEchoReply(uint32_t ip)
{
    InternetControlMessageProtocolMessage icmp;
    icmp.type = 8; // ping
    icmp.code = 0;
    icmp.data = 0x3713; // 1337
    icmp.checksum = 0;
    icmp.checksum = IPV4Protocol::Checksum((uint16_t*)&icmp,
        sizeof(InternetControlMessageProtocolMessage));
    
    backend->ipv4->Send(ip, 0x01, (uint8_t*)&icmp, sizeof(InternetControlMessageProtocolMessage));
}