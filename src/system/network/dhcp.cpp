#include <system/network/dhcp.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

#define PACKET_END(p) (((uint8_t*)(p)) + sizeof(DhcpHeader))
#define PACKET_INSIDE(i,p) (((uint8_t*)(i)) >= ((uint8_t*)(p)) && ((uint8_t*)(i)) <= PACKET_END(p))

void printf(char*);
void printfHex(uint8_t);
void printfHex16(uint16_t);
void printfHex32(uint32_t);
void PrintIP(uint32_t ip);

DHCP::DHCP(NetworkManager* backend)
{
    this->backend = backend;

    //Start socket
    this->dhcpSocket = backend->ipv4Handler->udpHandler->Connect(0xFFFFFFFF, 67);
    //Set Correct port
    this->dhcpSocket->localPort = 68;
}

void DHCP::EnableDHCP()
{
    DhcpHeader* header = (DhcpHeader*)MemoryManager::activeMemoryManager->malloc(sizeof(DhcpHeader));
    MemoryOperations::memset(header, 0, sizeof(DhcpHeader));

    header->opcode = 1;
    header->htype = HARDWARE_TYPE_ETHERNET;
    header->hlen = 6;
    header->hopCount = 0;
    header->xid = Convert::ByteSwap((uint32_t) 0);
    header->secCount = Convert::ByteSwap((uint32_t) 0);
    header->flags = Convert::ByteSwap((uint16_t)( 0 | 0x8000 ));

    MemoryOperations::memcpy(header->clientEthAddr, backend->MAC, 6);

    uint8_t* options = header->options;

    *((uint32_t*)options) = Convert::ByteSwap((uint32_t)MAGIC_COOKIE);
    options += 4;

    // First option, message type = DHCP_DISCOVER/DHCP_REQUEST
    *(options++) = 53;
    *(options++) = 1;
    *(options++) = DHCP_DISCOVER;

    // Client identifier
    *(options++) = 61;
    *(options++) = 0x07;
    *(options++) = 0x01;

    //get_mac_addr(options);
    *(options++) = backend->MAC[5];
    *(options++) = backend->MAC[4];
    *(options++) = backend->MAC[3];
    *(options++) = backend->MAC[2];
    *(options++) = backend->MAC[1];
    *(options++) = backend->MAC[0];

    options += 6;

    // Requested IP address
    *(options++) = 50;
    *(options++) = 0x04;
    *((uint32_t*)(options)) = Convert::ByteSwap((uint32_t)0x0a00020e);
    //memcpy((uint32_t*)(options), request_ip, 4);
    options += 4;

    // Host Name
    *(options++) = 12;
    *(options++) = 0x09;
    MemoryOperations::memcpy(options, "CactusOS", 8);
    options += 8;
    *(options++) = 0x00;

    // Parameter request list
    *(options++) = 55;
    *(options++) = 8;
    *(options++) = 0x1;
    *(options++) = 0x3;
    *(options++) = 0x6;
    *(options++) = 0xf;
    *(options++) = 51; //Lease time
    *(options++) = 0x2c;
    *(options++) = 0x2e;
    *(options++) = 0x2f;
    *(options++) = 0x39;
    *(options++) = 0xff;


    //Finally send the packet
    this->dhcpSocket->Send((uint8_t*)header, sizeof(DhcpHeader));
}

void DHCP::HandleUDP(common::uint8_t* data, common::uint32_t size)
{
    printf("Parsing DHCP data\n");

    DhcpHeader* header = (DhcpHeader*)data;
    if(header->opcode == OP_REPLY)
    {
        if (header->options[0] != 0x63)
		    return;
	    if (header->options[1] != 0x82)
		    return;
	    if (header->options[2] != 0x53)
		    return;
	    if (header->options[3] != 0x63)
            return;

        printf("Header seems valid\n");
        printf("    Our IP: "); PrintIP(Convert::ByteSwap(header->yourIpAddr)); printf("\n");
        this->OurIp = header->yourIpAddr; //Set ip address to system

        uint8_t *p = header->options + 4;
        while (PACKET_INSIDE(p, header) && (*p & 0xff) != 0xff) {
            switch (*p) {
            case DhcpOptionHostName:
                MemoryOperations::memcpy(this->HostName, p + 2, *(p + 1));
                printf("    Host Name: "); printf(this->HostName); printf("\n");
                break;
            case DhcpOptionDNS:
                this->Dns = *(uint32_t*)(p + 2);
                printf("    DNS: "); PrintIP(Convert::ByteSwap(this->Dns)); printf("\n");
                break;
            case DhcpOptionIPAddrLeaseTime:
                this->LeaseTime = Convert::ByteSwap(*(uint32_t*)(p + 2));
                printf("    Lease Time: "); printf(Convert::IntToString(this->LeaseTime)); printf("\n");
                break;
            case DhcpOptionRoutersOnSubnet:
                this->RouterIp = *(uint32_t*)(p + 2);
                printf("    Router IP: "); PrintIP(Convert::ByteSwap(this->RouterIp)); printf("\n");
                break;
            case DhcpOptionServerIdentifier:
                this->ServerIp = *(uint32_t*)(p + 2);
                printf("    Server IP: "); PrintIP(Convert::ByteSwap(this->ServerIp)); printf("\n");
                break;
            case DhcpOptionSubnetMask:
                this->SubnetMask = *(uint32_t*)(p + 2);
                printf("    Subnet Mask: "); PrintIP(Convert::ByteSwap(this->SubnetMask)); printf("\n");
                break;
            default:
                printf("    Option not used: "); printfHex(*p); printf("\n");
                break;
            }
            p++;
            p += *p + 1;
        }

        this->Enabled = true;
        this->dhcpSocket->Disconnect(); //we don't need it anymore
    }

    printf("DHCP Parsed!\n");
}