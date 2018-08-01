#include <system/network/udp.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::core;

void printf(char*);
void printfHex(uint8_t);
void printfHex16(uint16_t);
void printfHex32(uint32_t);

UDPSocket* sockets[1200]; //65535 is way to much right?
common::uint16_t numSockets;

//UDPSocket
UDPSocket::UDPSocket(UserDatagramProtocolManager* backend)
{
    this->backend = backend;
    this->receiveHandle = 0;
    this->listening = false;
}
UDPSocket::~UDPSocket()
{

}
void UDPSocket::HandleData(uint8_t* data, uint16_t size)
{
    printf("Passing data to receiveHandle\n");
    if(receiveHandle != 0)
        receiveHandle(data, size);
}
void UDPSocket::Send(uint8_t* data, uint16_t size)
{
    backend->Send(this, data, size);
}
void UDPSocket::Disconnect()
{
    backend->Disconnect(this);
}




UserDatagramProtocolManager::UserDatagramProtocolManager(IPV4Handler* backend)
{
    for(int i = 0; i < 1200; i++)
        sockets[i] = 0;
    numSockets = 0;
    freePort = 1024;
    this->backend = backend;
}

UserDatagramProtocolManager::~UserDatagramProtocolManager()
{

}

void PrintIP(uint32_t ip); //Defined in kernel


void UserDatagramProtocolManager::OnInternetProtocolReceived(uint32_t srcIP_BE, uint32_t dstIP_BE,
                                uint8_t* payload, uint32_t size)
{
    if(size < sizeof(UserDatagramProtocolHeader))
        return;
    
    UserDatagramProtocolHeader* msg = (UserDatagramProtocolHeader*)payload;
    uint16_t localPort = Convert::ByteSwap(msg->dstPort);
    uint16_t remotePort = Convert::ByteSwap(msg->srcPort);
    
    printf("Received UDP:\n");

    //printf((char*)(payload + sizeof(UserDatagramProtocolHeader)));

    UDPSocket* socket = 0;
    printf("Number of sockets: "); printf(Convert::IntToString(numSockets)); printf("\n");
    for(uint16_t i = 0; i < numSockets && socket == 0; i++)
    {
        printf("Trying socket: "); printf(Convert::IntToString(i)); printf("\n");
        printf("Message Data:\n");
        printf("LocalPort: "); printf(Convert::IntToString(localPort));
        printf("\nRemotePort: "); printf(Convert::IntToString(remotePort));
        printf("\nSize: "); printf(Convert::IntToString(Convert::ByteSwap(msg->length) - sizeof(UserDatagramProtocolHeader))); printf("\n");

        printf("Socket Data:\n");
        printf("Listening: "); printf(sockets[i]->listening ? (char*)"True\n" : (char*)"False\n");
        printf("Local IP: "); PrintIP(Convert::ByteSwap(dstIP_BE)); printf("\n");
        printf("Local Port: "); printf(Convert::IntToString(sockets[i]->localPort)); printf("\n");

        printf("Remote IP: "); PrintIP(Convert::ByteSwap(srcIP_BE)); printf("\n");
        printf("Remote Port: "); printf(Convert::IntToString(sockets[i]->remotePort)); printf("\n");

        UDPSocket* cur = sockets[i];
        if(cur->localPort == localPort
        && cur->localIP == dstIP_BE
        && cur->listening == true)
        {
            socket = cur;
            socket->listening = false;
            socket->remotePort = remotePort;
            socket->remoteIP = srcIP_BE;
        }
        
        else if( sockets[i]->localPort == localPort
        &&  cur->localIP == dstIP_BE
        &&  sockets[i]->remotePort == remotePort
        &&  sockets[i]->remoteIP == srcIP_BE)
            socket = sockets[i];

    }

    if(remotePort == 67)
    {
        printf("Packet is DHCP\n");
        for(uint16_t i = 0; i < numSockets; i++) //Do a special search for dhcp socket, this should be done in the loop above!
            if(sockets[i]->localPort == 68 && sockets[i]->remotePort == 67)
                socket = sockets[i];    
    }
    
    if(socket != 0)
        socket->HandleData(payload + sizeof(UserDatagramProtocolHeader),
        size - sizeof(UserDatagramProtocolHeader));
    else
        printf("Socket not found\n");
}

UDPSocket* UserDatagramProtocolManager::Connect(uint32_t ip, uint16_t port)
{
    UDPSocket* socket = new UDPSocket(this);
    
    if(socket != 0)
    {        
        socket -> remotePort = port;
        socket -> remoteIP = ip;
        socket -> localPort = freePort++;   
        socket -> localIP = backend->netManager->IP_BE;     
        //socket -> remotePort = ((socket -> remotePort & 0xFF00)>>8) | ((socket -> remotePort & 0x00FF) << 8);
        //socket -> localPort = ((socket -> localPort & 0xFF00)>>8) | ((socket -> localPort & 0x00FF) << 8);
        
        sockets[numSockets] = socket;
        numSockets++;
        printf("Connect: added socket\n");
        printf("Number of sockets: "); printf(Convert::IntToString(numSockets)); printf("\n");
    }
    
    return socket;
}
UDPSocket* UserDatagramProtocolManager::Listen(uint16_t port)
{
    UDPSocket* socket = new UDPSocket(this);
    
    if(socket != 0)
    {    
        socket -> listening = true;
        socket -> localPort = port;
        socket -> localIP = backend->netManager->IP_BE;   

        //socket -> localPort = ((socket -> localPort & 0xFF00)>>8) | ((socket -> localPort & 0x00FF) << 8);
        sockets[numSockets] = socket;
        numSockets++;
        printf("Listen: added socket\n");
        printf("Number of sockets: "); printf(Convert::IntToString(numSockets)); printf("\n");
    }
    return socket;
}
void UserDatagramProtocolManager::Disconnect(UDPSocket* socket)
{
    for(uint16_t i = 0; i < numSockets && socket == 0; i++)
        if(sockets[i] == socket)
        {
            sockets[i] = sockets[--numSockets];
            delete socket;
            break;
        }
}
void UserDatagramProtocolManager::Send(UDPSocket* socket, uint8_t* data, uint16_t size)
{
    uint16_t totalLength = size + sizeof(UserDatagramProtocolHeader);
    uint8_t* buffer = (uint8_t*)MemoryManager::activeMemoryManager->malloc(totalLength);
    uint8_t* buffer2 = buffer + sizeof(UserDatagramProtocolHeader);
    
    UserDatagramProtocolHeader* msg = (UserDatagramProtocolHeader*)buffer;
    
    msg->srcPort = Convert::ByteSwap(socket->localPort); //forgot this
    msg->dstPort = Convert::ByteSwap(socket->remotePort); //forgot this
    msg->length = ((totalLength & 0x00FF) << 8) | ((totalLength & 0xFF00) >> 8);
    
    for(int i = 0; i < size; i++)
        buffer2[i] = data[i];
    
    msg -> checksum = 0;
    this->backend->Send(socket->remoteIP, 0x11, buffer, totalLength);

    MemoryManager::activeMemoryManager->free(buffer);
}