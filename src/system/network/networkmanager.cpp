#include <system/network/networkmanager.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex16(uint16_t);

NetworkManager::NetworkManager(NetworkDriver* net_device)
{
    this->netDevice = net_device;
    this->netDevice->NetManager = this;
}
bool NetworkManager::StartNetwork(core::PIT* pit)
{
    //Initialize Handlers
    printf("Adding network handlers\n");
    printf("    -> ARP\n");
    this->arp = new ARPProtocol(this, pit);
    printf("    -> IPV4\n");
    this->ipv4 = new IPV4Protocol(this);
    printf("    -> ICMP\n");
    this->icmp = new InternetControlMessageProtocol(this);
    printf("    -> UDP\n");
    this->udp = new UserDatagramProtocolManager(this);
    printf("    -> DHCP\n");
    this->dhcp = new DHCP(this);

    printf("Trying automatic configuration via DHCP\n");
    uint32_t DhcpTries = 0;
    while(!dhcp->Enabled && DhcpTries < 5)
    {
        dhcp->SendDiscovery();
        pit->Sleep(500);
        DhcpTries++;
    }
    if(!dhcp->Enabled)
    {
        printf("Failed to do automatic dhcp connect\n");
        Console::Write("Do you want to do manual configuration? [y/n]: ");
        char answer = Console::ReadLine()[0];
        if(answer == 'y')
        {
            Console::WriteLine("Warning: this feature could potentialy harm your network!");
            Console::Write("Device MAC: "); NetTools::PrintMac(this->GetMACAddress()); Console::WriteLine();
            Console::Write("Our IP => ");
            this->Config.OurIp = NetTools::ParseIP(Console::ReadLine()); Console::WriteLine();
            Console::Write("Router IP => ");
            this->Config.RouterIp = NetTools::ParseIP(Console::ReadLine()); Console::WriteLine();
            this->Config.ServerIp = this->Config.RouterIp;
            Console::Write("Subnet Mask => ");
            this->Config.SubnetMask = NetTools::ParseIP(Console::ReadLine()); Console::WriteLine();
            Console::Write("DNS => ");
            this->Config.DnsIP = NetTools::ParseIP(Console::ReadLine()); Console::WriteLine();
            Console::Write("Hostname (optional) => ");
            char* hname = Console::ReadLine(); Console::WriteLine();
            MemoryOperations::memcpy(this->Config.HostName, hname, 100);
            Console::WriteLine("Manual network configuration done!");
            return true;
        }
        else
        {
            Console::WriteLine("Disabled Network");
            return true;
        }
    }
    else
    {
        printf("DHCP Is Enabled!\n");
        return true;
    }
}
void NetworkManager::HandleEthernetPacket(common::uint8_t* packet, common::uint32_t size)
{
    EtherFrameHeader* frame = (EtherFrameHeader*)packet;

    if(frame->dstMAC_BE == Convert::ByteSwap(this->netDevice->GetMacAddress()) || frame->dstMAC_BE == MAC_BROADCAST)
    {
        switch(Convert::ByteSwap(frame->etherType_BE))
        {
            case ETHERNET_TYPE_ARP:
                {
                    if(this->arp != 0)
                        this->arp->HandlePacket(packet + sizeof(EtherFrameHeader), size - sizeof(EtherFrameHeader));
                }
                break;
            case ETHERNET_TYPE_IP:
                {
                    if(this->ipv4 != 0)
                        this->ipv4->HandlePacket(packet + sizeof(EtherFrameHeader), size - sizeof(EtherFrameHeader));
                }
                break;
            default:
                printf("Unkown Ethernet packet type: "); printfHex16(Convert::ByteSwap(frame->etherType_BE)); printf("\n");
        }
    }
}
void NetworkManager::SendEthernetPacket(common::uint48_t dstMAC, common::uint16_t etherType, common::uint8_t* buffer, common::uint32_t size)
{
    if(size > 1518 - sizeof(EtherFrameHeader))
    {
        printf("Packet to big to send, so packet is ignored!\n");
        return;
    }
    else if(size < 64 - sizeof(EtherFrameHeader))
        size = 64 - sizeof(EtherFrameHeader); //Add extra bytes, this only for arp right now, otherwise it won't work

    uint8_t* buffer2 = (uint8_t*)MemoryManager::activeMemoryManager->malloc(sizeof(EtherFrameHeader) + size);
    MemoryOperations::memset(buffer2, 0, sizeof(EtherFrameHeader) + size);
    EtherFrameHeader* frame = (EtherFrameHeader*)buffer2;
    
    frame->dstMAC_BE = Convert::ByteSwap(dstMAC); //Byteswap MAC
    frame->srcMAC_BE = Convert::ByteSwap(netDevice->GetMacAddress()); //Byteswap MAC
    frame->etherType_BE = Convert::ByteSwap(etherType);

    uint8_t* src = buffer;
    uint8_t* dst = buffer2 + sizeof(EtherFrameHeader);
    for(uint32_t i = 0; i < size; i++)
        dst[i] = src[i];
    
    netDevice->SendData(buffer2, size + sizeof(EtherFrameHeader));
    MemoryManager::activeMemoryManager->free(buffer2);  
}
uint48_t NetworkManager::GetMACAddress()
{
    return this->netDevice->GetMacAddress();
}
uint8_t* NetworkManager::GetMACArray()
{
    static uint8_t mac[6];
    uint48_t key = GetMACAddress();

    mac[0] = key & 0xFF;
    mac[1] = ((key >> 8) & 0xFF);
    mac[2] = ((key >> 16) & 0xFF);
    mac[3] = ((key >> 24) & 0xFF);
    mac[4] = ((key >> 32) & 0xFF);
    mac[5] = ((key >> 40) & 0xFF);
    return mac;
}
uint32_t NetworkManager::GetIPAddress()
{
    if(this->Config.OurIp != 0)
        return this->Config.OurIp;
    return 0;
}