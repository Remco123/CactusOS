#include <system/network/ipv4.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);

IPV4Protocol::IPV4Protocol(NetworkManager* backend)
{
    this->netManager = backend;
}

void IPV4Protocol::HandlePacket(uint8_t* etherframePayload, uint32_t size)
{
    printf("IPV4 handling packet\n");
    InternetProtocolV4Message* ipmessage = (InternetProtocolV4Message*)etherframePayload;
    
    if(Convert::ByteSwap(ipmessage->dstIP) == netManager->GetIPAddress() || ipmessage->dstIP == IP_BROADCAST)
    {
        uint32_t length = ipmessage->totalLength;
        if(length > size)
            length = size;
        
        switch(ipmessage->protocol)
        {
            case 0x01: //icmp
                if(this->netManager->icmp != 0)
                    this->netManager->icmp->OnInternetProtocolReceived(Convert::ByteSwap(ipmessage->srcIP), Convert::ByteSwap(ipmessage->dstIP), etherframePayload + 4*ipmessage->headerLength, length - 4*ipmessage->headerLength);
                break;
            case 0x11: //udp
                if(this->netManager->udp != 0)
                    this->netManager->udp->OnInternetProtocolReceived(Convert::ByteSwap(ipmessage->srcIP), Convert::ByteSwap(ipmessage->dstIP), etherframePayload + 4*ipmessage->headerLength, length - 4*ipmessage->headerLength);
                break;
            case 0x06: //tcp
                if(this->netManager->tcp != 0)
                    this->netManager->tcp->OnInternetProtocolReceived(Convert::ByteSwap(ipmessage->srcIP), Convert::ByteSwap(ipmessage->dstIP), etherframePayload + 4*ipmessage->headerLength, length - 4*ipmessage->headerLength);
                break;
        }
    }
}
void IPV4Protocol::Send(uint32_t dstIP, uint8_t protocol, uint8_t* data, uint32_t size)
{
    uint8_t* buffer = (uint8_t*)MemoryManager::activeMemoryManager->malloc(sizeof(InternetProtocolV4Message) + size);
    MemoryOperations::memset(buffer, 0, sizeof(InternetProtocolV4Message) + size);
    InternetProtocolV4Message* message = (InternetProtocolV4Message*)buffer;
    
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
    
    message->dstIP = Convert::ByteSwap(dstIP);
    message->srcIP = Convert::ByteSwap(netManager->GetIPAddress());
    
    message->checksum = 0;
    message->checksum = Checksum((uint16_t*)message, sizeof(InternetProtocolV4Message));
    
    uint8_t* databuffer = buffer + sizeof(InternetProtocolV4Message);
    for(uint32_t i = 0; i < size; i++)
        databuffer[i] = data[i];
    
    uint32_t route = dstIP;

    if((dstIP & this->netManager->Config.SubnetMask) != (netManager->GetIPAddress() & this->netManager->Config.SubnetMask))
    {
        route = this->netManager->Config.RouterIp;
        printf("Modified route\n");
    }

    
    netManager->SendEthernetPacket(Convert::ByteSwap(netManager->arp->Resolve(route)), ETHERNET_TYPE_IP, buffer, sizeof(InternetProtocolV4Message) + size);
    
    MemoryManager::activeMemoryManager->free(buffer);   
}

uint16_t IPV4Protocol::Checksum(uint16_t* data, uint32_t lengthInBytes)
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