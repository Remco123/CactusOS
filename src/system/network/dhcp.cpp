#include <system/network/dhcp.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);
void printfHex16(uint16_t);
void printfHex32(uint32_t);


UDPSocket* DHCP::dhcpSocket = 0;

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
}