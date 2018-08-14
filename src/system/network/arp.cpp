#include <system/network/arp.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);
void printfHex16(uint16_t);
void PrintIP(uint32_t);

//Prints mac address
void PrintMac(uint64_t key)
{
    printfHex( key & 0xFF);
    printf(":");
    printfHex((key >> 8) & 0xFF);
    printf(":");
    printfHex((key >> 16) & 0xFF);
    printf(":");
    printfHex((key >> 24) & 0xFF);
    printf(":");
    printfHex((key >> 32) & 0xFF);
    printf(":");
    printfHex((key >> 40) & 0xFF);
}

AddressResolutionProtocol::AddressResolutionProtocol(NetworkManager* parent, PIT* pit)
{
    this->NumArpItems = 0;
    this->netManager = parent;
    this->pit = pit;
}
AddressResolutionProtocol::~AddressResolutionProtocol()
{

}

void AddressResolutionProtocol::HandlePacket(uint8_t* packet, uint32_t size)
{
    printf("Handling arp packet\n");
    AddressResolutionProtocolMessage* arp = (AddressResolutionProtocolMessage*)packet;

    printf("HW Type: "); printfHex16(arp->hardwareType); printf("\n");
    printf("srcIP: "); PrintIP(Convert::ByteSwap(arp->srcIP)); printf("\n");
    printf("dstIP: "); PrintIP(Convert::ByteSwap(arp->dstIP)); printf("\n");
    printf("Command: "); printfHex16(arp->command); printf("\n");
    
    if(arp->hardwareType == (uint16_t) 0x0100)
    {
        if(arp->protocol == 0x0008
        && arp->hardwareAddressSize == 6
        && arp->protocolAddressSize == 4
        && arp->dstIP == netManager->GetIPAddress())
        {
            switch(arp->command)
            {
                case 0x0100: // request
                    arp->command = 0x0200;
                    arp->dstIP = arp->srcIP;
                    arp->dstMAC = arp->srcMAC;
                    arp->srcIP = netManager->GetIPAddress();
                    arp->srcMAC = netManager->GetMACAddress();
                    
                    netManager->SendPacket(arp->dstMAC, Convert::ByteSwap((uint16_t)ETHERNET_TYPE_ARP), packet, size);
                    printf("Arp Response send\n");
                    break;
                    
                case 0x0200: // response
                    printf("Got arp response\n");
                    if(NumArpItems < 128)
                    {
                        ArpEntry* entry = new ArpEntry();
                        entry->MACAddress = arp->srcMAC;
                        entry->IPAddress = arp->srcIP;

                        ArpDatabase[NumArpItems] = entry;
                        NumArpItems++;
                        printf("Arp Entry added to database\n");
                        printf("MAC: "); PrintMac(entry->MACAddress); printf("\n");
                    }
                    else
                        printf("ARP Database is full!\n");
                    break;
            }
        }
    }
}

void AddressResolutionProtocol::RequestMAC(common::uint32_t IP_BE)
{
    AddressResolutionProtocolMessage arp;
    arp.hardwareType = 0x0100; // ethernet
    arp.protocol = 0x0008; // ipv4
    arp.hardwareAddressSize = 6; // mac
    arp.protocolAddressSize = 4; // ipv4
    arp.command = 0x0100; // request
    
    arp.srcMAC = netManager->GetMACAddress();
    arp.srcIP = netManager->GetIPAddress();
    arp.dstMAC = 0xFFFFFFFFFFFF; // broadcast
    arp.dstIP = IP_BE;

    netManager->SendPacket(arp.dstMAC, Convert::ByteSwap((uint16_t)ETHERNET_TYPE_ARP), (uint8_t*)&arp, sizeof(AddressResolutionProtocolMessage));
}

uint64_t AddressResolutionProtocol::Resolve(uint32_t IP_BE)
{
    if(IP_BE == 0xFFFFFFFF)
        return 0xFFFFFFFFFFFF;

    uint64_t result = GetMACFromCache(IP_BE);
    if(result == 0)
        RequestMAC(IP_BE);
    else
        return result; //It is already in the database
    
    for(int i = 0; i < MACResolveMaxTries; i++)
    {
        result = GetMACFromCache(IP_BE);
        if(result != 0)
            return result;
        printf("*");
        pit->Sleep(50); //Small timeout
    }
    printf("    :Arp Resolve: Request timed out\n");

    return 0;
}

uint64_t AddressResolutionProtocol::GetMACFromCache(uint32_t IP_BE)
{
    for(uint32_t i = 0; i < NumArpItems; i++)
        if(ArpDatabase[i]->IPAddress == IP_BE)
            return ArpDatabase[i]->MACAddress;
    return 0;
}

void AddressResolutionProtocol::BroadcastMACAddress(uint32_t IP_BE)
{
    AddressResolutionProtocolMessage arp;
    arp.hardwareType = 0x0100; // ethernet
    arp.protocol = 0x0008; // ipv4
    arp.hardwareAddressSize = 6; // mac
    arp.protocolAddressSize = 4; // ipv4
    arp.command = 0x0200; // "response"
    
    arp.srcMAC = netManager->GetMACAddress();
    arp.srcIP = netManager->GetIPAddress();
    arp.dstMAC = Resolve(IP_BE);
    arp.dstIP = IP_BE;
    
    netManager->SendPacket(arp.dstMAC, Convert::ByteSwap((uint16_t)ETHERNET_TYPE_ARP), (uint8_t*)&arp, sizeof(AddressResolutionProtocolMessage));

}