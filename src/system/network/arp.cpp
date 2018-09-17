#include <system/network/arp.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);
void printfHex16(uint16_t);

uint64_t ARPProtocol::GetMACFromDatabase(uint32_t ip)
{
    for(uint32_t i = 0; i < NumArpItems; i++)
        if(ArpDatabase[i]->IPAddress == ip)
            return ArpDatabase[i]->MACAddress;
    printf("MAC Not found in Arp database\n");
    return 0;
}


ARPProtocol::ARPProtocol(NetworkManager* parent, PIT* pit)
{
    this->NumArpItems = 0;
    this->netManager = parent;
    this->pit = pit;
}
void ARPProtocol::HandlePacket(uint8_t* packet, uint32_t size)
{
    AddressResolutionProtocolMessage* arp = (AddressResolutionProtocolMessage*)packet;
    
    if(arp->hardwareType == (uint16_t) 0x0100)
    {
        if(arp->protocol == 0x0008
        && arp->hardwareAddressSize == 6
        && arp->protocolAddressSize == 4
        && Convert::ByteSwap(arp->dstIP) == netManager->GetIPAddress())
        {
            printf("Handling arp packet\n");
            switch(arp->command)
            {
                case 0x0100: // request
                    SendResponse(Convert::ByteSwap(arp->srcMAC), Convert::ByteSwap(arp->srcIP));
                    printf("Arp Response Send\n");
                    break;
                    
                case 0x0200: // response
                    printf("Got arp response\n");
                    if(NumArpItems < 128)
                    {
                        ArpEntry* entry = new ArpEntry();
                        entry->MACAddress = arp->srcMAC;
                        entry->IPAddress = Convert::ByteSwap(arp->srcIP);

                        ArpDatabase[NumArpItems] = entry;
                        NumArpItems++;
                        printf("Arp Entry added to database\n");
                        printf("MAC/IP "); NetTools::PrintMac(entry->MACAddress); printf("/"); NetTools::PrintIP(entry->IPAddress); printf("\n");
                    }
                    else
                        printf("ARP Database is full!\n");
                    break;
            }
        }
    }
}
void ARPProtocol::SendRequest(uint32_t ip)
{
    AddressResolutionProtocolMessage arp;
    arp.hardwareType = 0x0100;
    arp.protocol = 0x008;
    arp.hardwareAddressSize = 6;
    arp.protocolAddressSize = 4;
    arp.command = 0x0100; //request
    arp.srcMAC = Convert::ByteSwap(netManager->GetMACAddress());
    arp.srcIP = Convert::ByteSwap(netManager->GetIPAddress());
    arp.dstMAC = Convert::ByteSwap((uint48_t)MAC_BROADCAST );
    arp.dstIP = Convert::ByteSwap(ip);
    netManager->SendEthernetPacket(arp.dstMAC, ETHERNET_TYPE_ARP, (uint8_t*)&arp, sizeof(AddressResolutionProtocolMessage));
}
void ARPProtocol::SendResponse(uint48_t TargetMAC, uint32_t TargetIP)
{
    AddressResolutionProtocolMessage arp;
    arp.hardwareType = 0x0100;
    arp.protocol = 0x008;
    arp.hardwareAddressSize = 6;
    arp.protocolAddressSize = 4;
    arp.command = 0x0200; //response

    arp.srcMAC = Convert::ByteSwap(netManager->GetMACAddress());
    arp.srcIP = Convert::ByteSwap(netManager->GetIPAddress());
    arp.dstMAC = Convert::ByteSwap(TargetMAC);
    arp.dstIP = Convert::ByteSwap(TargetIP);
    netManager->SendEthernetPacket(TargetMAC, ETHERNET_TYPE_ARP, (uint8_t*)&arp, sizeof(AddressResolutionProtocolMessage));
}
uint48_t ARPProtocol::Resolve(uint32_t ip)
{
    const int MaxTries = 5;

    if(ip == IP_BROADCAST)
        return MAC_BROADCAST;

    uint48_t result = GetMACFromDatabase(ip);
    if(result != 0)
        return result;

    for(int i = 0; i < MaxTries; i++)
    {
        SendRequest(ip);
        result = GetMACFromDatabase(ip);
        if(result != 0)
            return result;
        printf("*");
        pit->Sleep(200); //Small timeout
    }
    printf("    :Arp Resolve: Request timed out\n");
    return 0;
}

void ARPProtocol::PrintArpEntries()
{
    printf("Arp Database contents\n");
    for(uint32_t i = 0; i < NumArpItems; i++)
    {
        ArpEntry* entry = ArpDatabase[i];
        printf("-> MAC/IP:    "); NetTools::PrintMac(entry->MACAddress); printf("/"); NetTools::PrintIP(entry->IPAddress); printf("\n");
    }
}