#include <system/network/ipv4.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);

IPV4Handler::IPV4Handler(NetworkManager* backend)
{
    this->netManager = backend;
}
IPV4Handler::~IPV4Handler()
{

}

void IPV4Handler::HandlePacket(uint8_t* etherframePayload, uint32_t size)
{
    printf("IPV4 handling packet\n");
    InternetProtocolV4Message* ipmessage = (InternetProtocolV4Message*)etherframePayload;
    
    if(ipmessage->dstIP == netManager->GetIPAddress() || ipmessage->dstIP == 0xFFFFFFFF)
    {
        uint32_t length = ipmessage->totalLength;
        if(length > size)
            length = size;
        
        printf("Packet is for us joehoe!\n");
        switch(ipmessage->protocol)
        {
            case 0x01: //icmp
                if(this->netManager->icmp != 0)
                    this->netManager->icmp->OnInternetProtocolReceived(ipmessage->srcIP, ipmessage->dstIP, etherframePayload + 4*ipmessage->headerLength, length - 4*ipmessage->headerLength);
                break;
            case 0x11: //udp
                if(this->netManager->udp != 0)
                    this->netManager->udp->OnInternetProtocolReceived(ipmessage->srcIP, ipmessage->dstIP, etherframePayload + 4*ipmessage->headerLength, length - 4*ipmessage->headerLength);
                break;
        }
    }
}
void IPV4Handler::Send(uint32_t dstIP_BE, uint8_t protocol, uint8_t* data, uint32_t size)
{
    uint8_t* buffer = (uint8_t*)MemoryManager::activeMemoryManager->malloc(sizeof(InternetProtocolV4Message) + size);
    InternetProtocolV4Message *message = (InternetProtocolV4Message*)buffer;
    
    message->version = 4;
    message->headerLength = sizeof(InternetProtocolV4Message)/4;
    message->tos = 0;
    message->totalLength = size + sizeof(InternetProtocolV4Message);
    message->totalLength = ((message->totalLength & 0xFF00) >> 8)
                         | ((message->totalLength & 0x00FF) << 8);
    message->ident = 0x0100;
    message->flagsAndOffset = 0x0040;
    message->timeToLive = 0x40;
    message->protocol = protocol;
    
    message->dstIP = dstIP_BE;
    message->srcIP = netManager->GetIPAddress();
    
    message->checksum = 0;
    message->checksum = Checksum((uint16_t*)message, sizeof(InternetProtocolV4Message));
    
    uint8_t* databuffer = buffer + sizeof(InternetProtocolV4Message);
    for(uint32_t i = 0; i < size; i++)
        databuffer[i] = data[i];
    
    uint32_t route = dstIP_BE;
    if((dstIP_BE & this->netManager->dhcp->SubnetMask) != (message->srcIP & this->netManager->dhcp->SubnetMask))
    {
        route = this->netManager->dhcp->RouterIp;
        printf("Modified route\n");
    }
    
    netManager->SendPacket(netManager->arp->Resolve(route), Convert::ByteSwap((uint16_t)ETHERNET_TYPE_IP), buffer, sizeof(InternetProtocolV4Message) + size);
    
    MemoryManager::activeMemoryManager->free(buffer);   
}

uint16_t IPV4Handler::Checksum(uint16_t* data, uint32_t lengthInBytes)
{
    uint32_t temp = 0;

    for(uint32_t i = 0; i < lengthInBytes/2; i++)
        temp += ((data[i] & 0xFF00) >> 8) | ((data[i] & 0x00FF) << 8);

    if(lengthInBytes % 2)
        temp += ((uint16_t)((char*)data)[lengthInBytes-1]) << 8;
    
    while(temp & 0xFFFF0000)
        temp = (temp & 0xFFFF) + (temp >> 16);
    
    return ((~temp & 0xFF00) >> 8) | ((~temp & 0x00FF) << 8);
}