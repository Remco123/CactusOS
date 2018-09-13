#include <system/network/networkmanager.h>

using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);
void printfHex16(uint16_t);
void printfHex32(uint32_t);
void PrintIP(uint32_t ip);
void PrintMac(uint64_t key);

NetworkManager::NetworkManager(NetworkDriver* device)
{
    this->netDevice = device;
    this->netDevice->NetManager = this;
    this->NetworkAvailable = false;
}
NetworkManager::~NetworkManager()
{

}

uint32_t NetworkManager::ParseIP(char* str)
{
    char* parts[4];
    int partcount = 0;
    parts[partcount++] = str;

    while(*str)
    {
        if(*str == '.')
        {
            *str = 0;
            parts[partcount++] = str + 1;
        }
        str++;
    }

    uint8_t ip1 = Convert::StringToInt(parts[0]), ip2 = Convert::StringToInt(parts[1]), ip3 = Convert::StringToInt(parts[2]), ip4 = Convert::StringToInt(parts[3]);
    return ((uint32_t)ip1 << 24)
        |  ((uint32_t)ip2 << 16)
        |  ((uint32_t)ip3 << 8)
        |  (uint32_t)ip4;
}

//Start the network Stack
void NetworkManager::StartNetwork(core::PIT* pit)
{
    MemoryOperations::memcpy(this->MAC, this->netDevice->MAC, 6); //First save our mac address

    //Initialize Handlers
    printf("Adding network handlers\n");
    printf("    -> ARP\n");
    this->arp = new AddressResolutionProtocol(this, pit);
    printf("    -> IPV4\n");
    this->ipv4 = new IPV4Handler(this);
    printf("    -> ICPM\n");
    this->icmp = new InternetControlMessageProtocol(this);
    printf("    -> UDP\n");
    this->udp = new UserDatagramProtocolManager(this);
    printf("    -> DHCP\n");
    this->dhcp = new DHCP(this);
    
    printf("Trying automatic configuration via DHCP\n");
    uint32_t DhcpTries = 0;
    while(!dhcp->Enabled && DhcpTries < 5)
    {
        dhcp->EnableDHCP();
        pit->Sleep(500);
        DhcpTries++;
    }
    if(dhcp->Enabled)
    {
        printf("DHCP is configured!\n");
        this->NetworkAvailable = true;
    }
    else
    {
        printf("Failed to do automatic dhcp connect\n");
        Console::Write("Do you want to do manual configuration? [y/n]: ");
        char answer = Console::ReadLine()[0];
        if(answer == 'y')
        {
            Console::WriteLine("Warning: this feature could potentialy harm your network!");
            Console::Write("Device MAC: "); PrintMac(this->GetMACAddress()); Console::WriteLine();
            Console::Write("Our IP => ");
            this->dhcp->OurIp = ParseIP(Console::ReadLine()); Console::WriteLine();
            Console::Write("Router IP => ");
            this->dhcp->RouterIp = ParseIP(Console::ReadLine()); Console::WriteLine();
            this->dhcp->ServerIp = this->dhcp->RouterIp;
            Console::Write("Subnet Mask => ");
            this->dhcp->SubnetMask = ParseIP(Console::ReadLine()); Console::WriteLine();
            Console::Write("DNS => ");
            this->dhcp->Dns = ParseIP(Console::ReadLine()); Console::WriteLine();
            Console::Write("Hostname (optional) => ");
            char* hname = Console::ReadLine(); Console::WriteLine();
            MemoryOperations::memcpy(this->dhcp->HostName, hname, 100);
            Console::WriteLine("Manual network configuration done!");
            this->NetworkAvailable = true;
        }
        else
        {
            this->NetworkAvailable = false;
            Console::WriteLine("Disabled Network");
        }
    }
    if(this->NetworkAvailable)
    {
        //Just for testing
        uint64_t mac = this->arp->Resolve(this->dhcp->ServerIp);
        printf("Router IP: "); PrintIP(Convert::ByteSwap(this->dhcp->ServerIp)); printf("\n");
        printf("Router MAC: "); PrintMac(mac); printf("\n");
    }
}

//Raw data received by the network driver
void NetworkManager::HandlePacket(common::uint8_t* packet, common::uint32_t size)
{
    EtherFrameHeader* frame = (EtherFrameHeader*)packet;
    
    if(frame->dstMAC_BE == 0xFFFFFFFFFFFF || frame->dstMAC_BE == netDevice->GetMacAddressBE())
    {
        switch(Convert::ByteSwap(frame->etherType_BE))
        {
            case ETHERNET_TYPE_ARP:
                if(this->arp != 0)
                    this->arp->HandlePacket(packet + sizeof(EtherFrameHeader), size - sizeof(EtherFrameHeader));
                break;
            case ETHERNET_TYPE_IP:
                if(this->ipv4 != 0)
                    this->ipv4->HandlePacket(packet + sizeof(EtherFrameHeader), size - sizeof(EtherFrameHeader));
                break;
            default:
                printf("Unkown Ethernet packet type: "); printfHex16(Convert::ByteSwap(frame->etherType_BE)); printf("\n");
        }
    }
}

void PrintPacket(uint8_t* data, uint32_t size)
{
    Console::WriteLine("--------------# Packet #--------------");

    int x = 0;
    for(int i = 0; i < size; i++)
    {
        if(x == 16)
        {
            Console::ReadLine();
            x = 0;
        }
        printfHex(data[i]);
        if(i < size)
            printf(" ");
        x++;
    }

    Console::WriteLine();
    Console::WriteLine("-----------# End Of Packet #----------");
}

//Send raw data to the network device
void NetworkManager::SendPacket(common::uint64_t dstMAC_BE, common::uint16_t etherType_BE, common::uint8_t* buffer, common::uint32_t size)
{
    uint8_t* buffer2 = (uint8_t*)MemoryManager::activeMemoryManager->malloc(sizeof(EtherFrameHeader) + size);
    MemoryOperations::memset(buffer2, 0, sizeof(EtherFrameHeader) + size);
    EtherFrameHeader* frame = (EtherFrameHeader*)buffer2;
    
    frame->dstMAC_BE = dstMAC_BE;
    frame->srcMAC_BE = netDevice->GetMacAddressBE();
    frame->etherType_BE = etherType_BE;

    uint8_t* src = buffer;
    uint8_t* dst = buffer2 + sizeof(EtherFrameHeader);
    for(uint32_t i = 0; i < size; i++)
        dst[i] = src[i];
    
    netDevice->SendData(buffer2, size + sizeof(EtherFrameHeader));
    MemoryManager::activeMemoryManager->free(buffer2);
}

uint32_t NetworkManager::GetIPAddress()
{
    if(this->dhcp != 0)
        return this->dhcp->OurIp; //Return the IP gathered by DHCP
    else
    {
        printf("Warning DHCP is zero! So returning 0\n");
        return 0;
    }
}
uint64_t NetworkManager::GetMACAddress()
{
    return this->netDevice->GetMacAddressBE();
}

uint32_t NetworkManager::MakeIP(uint8_t* src)
{
    uint32_t ip_be = ((uint32_t)src[0] << 24)
                | ((uint32_t)src[1] << 16)
                | ((uint32_t)src[2] << 8)
                | (uint32_t)src[3];
    return Convert::ByteSwap(ip_be);
}