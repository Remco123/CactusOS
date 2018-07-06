#include <system/network/networkmanager.h>

using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);
void printfHex16(uint16_t);
void printfHex32(uint32_t);

NetworkManager* NetworkManager::instance = 0;

NetworkManager::NetworkManager(NetworkDriver* device)
{
    this->instance = this;
    this->netDevice = device;
    device->NetManager = this;
}
NetworkManager::~NetworkManager()
{

}
//Raw data received by the network driver
void NetworkManager::HandlePacket(common::uint8_t* packet, common::uint32_t size)
{
    EtherFrameHeader* frame = (EtherFrameHeader*)packet;
    
    if(frame->dstMAC_BE == 0xFFFFFFFFFFFF || frame->dstMAC_BE == netDevice->GetMacAddress())
    {
        printf("Received Packet for us!\n");
    }
}
//Send raw data to the network device
void NetworkManager::SendPacket(common::uint64_t dstMAC_BE, common::uint16_t etherType_BE, common::uint8_t* buffer, common::uint32_t size)
{
    uint8_t* buffer2 = (uint8_t*)MemoryManager::activeMemoryManager->malloc(sizeof(EtherFrameHeader) + size);
    EtherFrameHeader* frame = (EtherFrameHeader*)buffer2;
    
    frame->dstMAC_BE = dstMAC_BE;
    frame->srcMAC_BE = netDevice->GetMacAddress();
    frame->etherType_BE = etherType_BE;
    
    uint8_t* src = buffer;
    uint8_t* dst = buffer2 + sizeof(EtherFrameHeader);
    for(uint32_t i = 0; i < size; i++)
        dst[i] = src[i];
    
    netDevice->SendData(buffer2, size + sizeof(EtherFrameHeader));
}