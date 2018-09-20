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

DHCP::DHCP(NetworkManager* backend)
{
    this->backend = backend;

    //Start socket
    this->dhcpSocket = backend->udp->Connect(0xFFFFFFFF, 67);
    //Set Correct port
    this->dhcpSocket->localPort = 68;
    this->Enabled = false;
}

void DHCP::HandleUDP(common::uint8_t* data, common::uint32_t size)
{
    printf("Parsing DHCP Header\n");
    DhcpHeader* header = (DhcpHeader*)data;

    if(header->opcode == OP_REPLY)
    {
        printf("Got DHCP Reply\n");
        if (header->options[0] != 0x63)
		    return;
	    if (header->options[1] != 0x82)
		    return;
	    if (header->options[2] != 0x53)
		    return;
	    if (header->options[3] != 0x63)
            return;

        printf("Header seems valid\n");
        printf("Message Type: ");
        uint8_t MessageType = GetDHCPMessageType(data);
        printf(this->getTypeString(MessageType)); printf("\n");
        switch(MessageType)
        {
            case DHCP_OFFER:
                {
                    uint8_t requestIP[4] = { (header->yourIpAddr & 0x000000FF), (header->yourIpAddr & 0x0000FF00) >> 8, (header->yourIpAddr & 0x00FF0000) >> 16, (header->yourIpAddr & 0xFF000000) >> 24 };
                    uint8_t serverIP[4] =  { (header->serverIpAddr & 0x000000FF), (header->serverIpAddr & 0x0000FF00) >> 8, (header->serverIpAddr & 0x00FF0000) >> 16, (header->serverIpAddr & 0xFF000000) >> 24 };
                    SendRequest(requestIP, serverIP);
                    break;
                }
            case DHCP_ACK:
                {
                    printf("Parsing ACK!\n");
                    ParseACK(data);
                    printf("DHCP Enabled!\n");
                    this->Enabled = true;
                    break;
                }
            case DHCP_NAK:
                {
                    printf("Received NAK for IP: "); NetTools::PrintIP(header->yourIpAddr); printf("\n");
                    char* msg = getNAKMessage(data);
                    printf("Reason => "); printf(msg); printf("\n");
                    delete msg;
                }
        }
    }
    else if(header->opcode == OP_REQUEST)
    {
        printf("Got DHCP Request\n");
    }
}

void DHCP::SendDiscovery()
{
    DhcpHeader* header = (DhcpHeader*)MemoryManager::activeMemoryManager->malloc(sizeof(DhcpHeader));
    MemoryOperations::memset(header, 0, sizeof(DhcpHeader));

    header->opcode = 1;
    header->htype = 0x1;
    header->hlen = 6;
    header->hopCount = 0;
    header->xid = Convert::ByteSwap((uint32_t) 0);
    header->secCount = Convert::ByteSwap((uint32_t) 0);
    header->flags = Convert::ByteSwap((uint16_t)( 0 | 0x8000 ));

    for(int i = 0; i < 6; i++)
        header->clientEthAddr[i] = backend->GetMACArray()[5 - i];

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

    uint8_t* OurMAC = backend->GetMACArray();
    *(options++) = OurMAC[5];
    *(options++) = OurMAC[4];
    *(options++) = OurMAC[3];
    *(options++) = OurMAC[2];
    *(options++) = OurMAC[1];
    *(options++) = OurMAC[0];

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
    this->dhcpSocket->Send((uint8_t*)header, sizeof(DhcpHeader));
    MemoryManager::activeMemoryManager->free(header);
}
void DHCP::SendRequest(uint8_t* requestIP, uint8_t* serverIP)
{
    DhcpHeader* header = (DhcpHeader*)MemoryManager::activeMemoryManager->malloc(sizeof(DhcpHeader));
    MemoryOperations::memset(header, 0, sizeof(DhcpHeader));

    header->opcode = 1;
    header->htype = 0x1;
    header->hlen = 6;
    header->hopCount = 0;
    header->xid = Convert::ByteSwap((uint32_t) 0);
    header->secCount = Convert::ByteSwap((uint32_t) 0);
    header->flags = Convert::ByteSwap((uint16_t)( 0 | 0x8000 ));

    for(int i = 0; i < 6; i++)
        header->clientEthAddr[i] = backend->GetMACArray()[5 - i];

    uint8_t* options = header->options;

    *((uint32_t*)options) = Convert::ByteSwap((uint32_t)MAGIC_COOKIE);
    options += 4;

    // First option, message type = DHCP_DISCOVER/DHCP_REQUEST
    *(options++) = 53;
    *(options++) = 1;
    *(options++) = DHCP_REQUEST;

    // Client identifier
    *(options++) = 61;
    *(options++) = 0x07;
    *(options++) = 0x01;

    uint8_t* OurMAC = backend->GetMACArray();
    *(options++) = OurMAC[5];
    *(options++) = OurMAC[4];
    *(options++) = OurMAC[3];
    *(options++) = OurMAC[2];
    *(options++) = OurMAC[1];
    *(options++) = OurMAC[0];

    options += 6;

    // Requested IP address
    *(options++) = 50;
    *(options++) = 0x04;

    *(options++) = requestIP[0];
    *(options++) = requestIP[1];
    *(options++) = requestIP[2];
    *(options++) = requestIP[3];

    // Server Identifier
    *(options++) = 54;
    *(options++) = 0x04;
    *(options++) = serverIP[0];
    *(options++) = serverIP[1];
    *(options++) = serverIP[2];
    *(options++) = serverIP[3];

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
    this->dhcpSocket->Send((uint8_t*)header, sizeof(DhcpHeader));
    MemoryManager::activeMemoryManager->free(header);
}


void DHCP::ParseACK(unsigned char* data)
{
    DhcpHeader* header = (DhcpHeader*)data;
    printf("    Our IP: "); NetTools::PrintIP(Convert::ByteSwap(header->yourIpAddr)); printf("\n");

    this->backend->Config.OurIp = Convert::ByteSwap( header->yourIpAddr ); //Set ip address to system

    uint8_t *p = header->options + 4;
    while (PACKET_INSIDE(p, header) && (*p & 0xff) != 0xff) {
        switch (*p) {
        case DhcpOptionHostName:
            MemoryOperations::memcpy(this->backend->Config.HostName, p + 2, *(p + 1));
            printf("    Host Name: "); printf(this->backend->Config.HostName); printf("\n");
            break;
        case DhcpOptionDNS:
            this->backend->Config.DnsIP = Convert::ByteSwap( *(uint32_t*)(p + 2) );
            printf("    DNS: "); NetTools::PrintIP(this->backend->Config.DnsIP); printf("\n");
            break;
        case DhcpOptionIPAddrLeaseTime:
            this->backend->Config.LeaseTime = Convert::ByteSwap(*(uint32_t*)(p + 2));
            printf("    Lease Time: "); printf(Convert::IntToString(this->backend->Config.LeaseTime)); printf("\n");
            break;
        case DhcpOptionRoutersOnSubnet:
            this->backend->Config.RouterIp = Convert::ByteSwap( *(uint32_t*)(p + 2) );
            printf("    Router IP: "); NetTools::PrintIP(this->backend->Config.RouterIp); printf("\n");
            break;
        case DhcpOptionServerIdentifier:
            this->backend->Config.ServerIp = Convert::ByteSwap( *(uint32_t*)(p + 2) );
            printf("    Server IP: "); NetTools::PrintIP(this->backend->Config.ServerIp); printf("\n");
            break;
        case DhcpOptionSubnetMask:
            this->backend->Config.SubnetMask = Convert::ByteSwap( *(uint32_t*)(p + 2) );
            printf("    Subnet Mask: "); NetTools::PrintIP(this->backend->Config.SubnetMask); printf("\n");
            break;
        default:
            printf("    Option not used: "); printfHex(*p); printf("\n");
            break;
        }
        p++;
        p += *p + 1;
    }
    if(this->backend->Config.RouterIp == 0 && this->backend->Config.ServerIp != 0)
        this->backend->Config.RouterIp = this->backend->Config.ServerIp; //If we only get the server ip
    else if(this->backend->Config.RouterIp != 0 && this->backend->Config.ServerIp == 0)
        this->backend->Config.ServerIp = this->backend->Config.RouterIp; //If we only get the router ip
}

char* DHCP::getNAKMessage(unsigned char* data)
{
    DhcpHeader* header = (DhcpHeader*)data;
    uint8_t *p = header->options + 4;
    while (PACKET_INSIDE(p, header) && (*p & 0xff) != 0xff) {
        switch (*p) {
        case DhcpOptionMsg:
            uint8_t len = *(uint8_t*)(p + 1);
            char* msg = new char[len];
            MemoryOperations::memcpy(msg, (char*)(p + 2), len);
            msg[len] = '\0';
            return msg;            
        }
        p++;
        p += *p + 1;
    }
}

uint8_t DHCP::GetDHCPMessageType(unsigned char* data)
{
    DhcpHeader* header = (DhcpHeader*)data;
    uint8_t *p = header->options + 4;
    while (PACKET_INSIDE(p, header) && (*p & 0xff) != 0xff) {
        switch (*p) {
        case DhcpOptionMessageType:
            return *(uint8_t*)(p + 2);            
        }
        p++;
        p += *p + 1;
    }
    return DHCP_NAK;
}
char* DHCP::getTypeString(uint8_t type)
{
    switch(type)
    {
        case DHCP_ACK:
            return "ACK";
        case DHCP_DECLINE:
            return "DECLINE";
        case DHCP_DISCOVER:
            return "DISCOVER";
        case DHCP_INFORM:
            return "INFORM";
        case DHCP_NAK:
            return "NAK";
        case DHCP_OFFER:
            return "OFFER";
        case DHCP_RELEASE:
            return "RELEASE";
        case DHCP_REQUEST:
            return "REQUEST";
        default:
            return "UNKOWN";
    }
    return "UNKOWN";
}