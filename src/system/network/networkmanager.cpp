#include <system/network/networkmanager.h>

using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);
void printfHex16(uint16_t);
void printfHex32(uint32_t);

NetworkManager* NetworkManager::instance = 0;

NetworkManager::NetworkManager(NetworkDriver* device, uint32_t ip_BE)
{
    this->instance = this;
    this->netDevice = device;
    this->IP_BE = ip_BE;
    device->NetManager = this;

    this->arpHandler = new AddressResolutionProtocol(this);
}
NetworkManager::~NetworkManager()
{

}
//Raw data received by the network driver
void NetworkManager::HandlePacket(common::uint8_t* packet, common::uint32_t size)
{
    printf("Received Packet\n");
    EtherFrameHeader* frame = (EtherFrameHeader*)packet;
    
    if(frame->dstMAC_BE == 0xFFFFFFFFFFFF || frame->dstMAC_BE == netDevice->GetMacAddressBE())
    {
        printf("Received Packet that is for us!\n");
        switch(Convert::ByteSwap(frame->etherType_BE))
        {
            case ETHERNET_TYPE_ARP:
                if(arpHandler != 0)
                    arpHandler->HandlePacket(packet + sizeof(EtherFrameHeader), size - sizeof(EtherFrameHeader));
                break;
            case ETHERNET_TYPE_IP:
                printf("GOT IPv4 Packet\n");
                break;
            default:
                printf("Unkown Ethernet packet\n");
        }
    }
}

void PrintMac(uint64_t key);

//Send raw data to the network device
void NetworkManager::SendPacket(common::uint64_t dstMAC_BE, common::uint16_t etherType_BE, common::uint8_t* buffer, common::uint32_t size)
{
    uint8_t* buffer2 = (uint8_t*)MemoryManager::activeMemoryManager->malloc(sizeof(EtherFrameHeader) + size);
    EtherFrameHeader* frame = (EtherFrameHeader*)buffer2;
    
    frame->dstMAC_BE = dstMAC_BE;
    frame->srcMAC_BE = netDevice->GetMacAddressBE();
    frame->etherType_BE = etherType_BE;

    printf("## Our mac (BE): "); PrintMac(frame->srcMAC_BE); printf("\n");
    
    uint8_t* src = buffer;
    uint8_t* dst = buffer2 + sizeof(EtherFrameHeader);
    for(uint32_t i = 0; i < size; i++)
        dst[i] = src[i];
    
    netDevice->SendData(buffer2, size + sizeof(EtherFrameHeader));
    MemoryManager::activeMemoryManager->free(buffer2);
}

uint32_t NetworkManager::GetIPAddress()
{
    return this->IP_BE;
}
uint64_t NetworkManager::GetMACAddress()
{
    return this->netDevice->GetMacAddressBE();
}