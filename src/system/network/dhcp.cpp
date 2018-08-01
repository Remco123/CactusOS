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

UDPSocket* DHCP::dhcpSocket = 0;
bool DHCP::Enabled = false;

char DHCP::HostName[]     = {};
uint32_t DHCP::Dns          = 0;
uint32_t DHCP::ServerIp     = 0;
uint32_t DHCP::OurIp        = 0;
uint32_t DHCP::SubnetMask   = 0;
uint32_t DHCP::RouterIp     = 0;
uint32_t DHCP::LeaseTime    = 0;

void DHCP::EnableDHCP()
{
    //Start socket
    DHCP::dhcpSocket = System::networkManager->ipv4Handler->udpHandler->Connect(0xFFFFFFFF, 67);
    //Set Correct port
    DHCP::dhcpSocket->localPort = 68;
    //Set handler
    DHCP::dhcpSocket->receiveHandle = DHCP::HandleUDP;

    DhcpHeader* header = (DhcpHeader*)MemoryManager::activeMemoryManager->malloc(sizeof(DhcpHeader));
    MemoryOperations::memset(header, 0, sizeof(DhcpHeader));

    header->opcode = 1;
    header->htype = HARDWARE_TYPE_ETHERNET;
    header->hlen = 6;
    header->hopCount = 0;
    header->xid = Convert::ByteSwap((uint32_t) 0);
    header->secCount = Convert::ByteSwap((uint32_t) 0);
    header->flags = Convert::ByteSwap((uint16_t)( 0 | 0x8000 ));

    MemoryOperations::memcpy(header->clientEthAddr, System::networkManager->MAC, 6);

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
    *(options++) = System::networkManager->MAC[5];
    *(options++) = System::networkManager->MAC[4];
    *(options++) = System::networkManager->MAC[3];
    *(options++) = System::networkManager->MAC[2];
    *(options++) = System::networkManager->MAC[1];
    *(options++) = System::networkManager->MAC[0];

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
    DHCP::dhcpSocket->Send((uint8_t*)header, sizeof(DhcpHeader));
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

        uint8_t *p = header->options + 4;
        while (PACKET_INSIDE(p, header) && (*p & 0xff) != 0xff) {
            switch (*p) {
            case DhcpOptionHostName:
                MemoryOperations::memcpy(DHCP::HostName, p + 2, *(p + 1));
                printf("    Host Name: "); printf(DHCP::HostName); printf("\n");
                break;
            case DhcpOptionDNS:
                DHCP::Dns = *(uint32_t*)(p + 2);
                printf("    DNS: "); PrintIP(Convert::ByteSwap(DHCP::Dns)); printf("\n");
                break;
            case DhcpOptionIPAddrLeaseTime:
                DHCP::LeaseTime = Convert::ByteSwap(*(uint32_t*)(p + 2));
                printf("    Lease Time: "); printf(Convert::IntToString(DHCP::LeaseTime)); printf("\n");
                break;
            case DhcpOptionRoutersOnSubnet:
                DHCP::RouterIp = *(uint32_t*)(p + 2);
                printf("    Router IP: "); PrintIP(Convert::ByteSwap(DHCP::RouterIp)); printf("\n");
                break;
            case DhcpOptionServerIdentifier:
                DHCP::ServerIp = *(uint32_t*)(p + 2);
                printf("    Server IP: "); PrintIP(Convert::ByteSwap(DHCP::ServerIp)); printf("\n");
                break;
            case DhcpOptionSubnetMask:
                DHCP::SubnetMask = *(uint32_t*)(p + 2);
                printf("    Subnet Mask: "); PrintIP(Convert::ByteSwap(DHCP::SubnetMask)); printf("\n");
                break;
            default:
                printf("    Option not used: "); printfHex(*p); printf("\n");
                break;
            }
            p++;
            p += *p + 1;
        }

        DHCP::Enabled = true;
    }

    printf("DHCP Parsed!\n");
}