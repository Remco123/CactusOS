#include <system/network/tcp.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);
void printfHex16(uint16_t);
void printfHex32(uint32_t);

TCPSocket::TCPSocket(TransmissionControlProtocolManager* backend)
{
    this->backend = backend;
    this->state = TCPSocketState::CLOSED;
}
TCPSocket::~TCPSocket()
{
}
bool TCPSocket::HandleTCPData(common::uint8_t* data, common::uint32_t size)
{
    if(receiveHandle != 0)
        receiveHandle(data, size);
}
void TCPSocket::Send(common::uint8_t* data, common::uint32_t size)
{
    while(state != ESTABLISHED);
    backend->Send(this, data, size, PSH|ACK);
}
void TCPSocket::Disconnect()
{
    backend->Disconnect(this);
}





TransmissionControlProtocolManager::TransmissionControlProtocolManager(NetworkManager* backend)
{
    for(int i = 0; i < 65535; i++)
        sockets[i] = 0;
    numSockets = 0;
    freePort = 1024;
    this->backend = backend;
}

void TransmissionControlProtocolManager::OnInternetProtocolReceived(common::uint32_t srcIP, common::uint32_t dstIP, common::uint8_t* payload, common::uint32_t size)
{
    printf("Handling TCP Message\n");
    TCPHeader* msg = (TCPHeader*)payload;    

    uint16_t srcPort = Convert::ByteSwap(msg->srcPort);
    uint16_t dstPort = Convert::ByteSwap(msg->dstPort);

    printf("SrcPORT: "); printf(Convert::IntToString(srcPort)); printf("\n");
    printf("DstPORT: "); printf(Convert::IntToString(dstPort)); printf("\n");
    printf("SrcIP: "); NetTools::PrintIP(srcIP); printf("\n");
    printf("DstIP: "); NetTools::PrintIP(dstIP); printf("\n");

    TCPSocket* socket = 0;
    for(uint16_t i = 0; i < numSockets && socket == 0; i++)
    {
        if( sockets[i]->localPort == dstPort
        &&  sockets[i]->localIP == dstIP
        &&  sockets[i]->state == LISTEN
        && (((msg -> flags) & (SYN | ACK)) == SYN))
            socket = sockets[i];
        else if( sockets[i]->localPort == dstPort
        &&  sockets[i]->localIP == dstIP
        &&  sockets[i]->remotePort == srcPort
        &&  sockets[i]->remoteIP == srcIP)
            socket = sockets[i];
    }

    bool reset = false;

    if(socket != 0)
        printf("Found according socket!\n");

    if(socket != 0 && msg->flags & RST)
        socket->state = CLOSED;
    
    if(socket != 0 && socket->state != CLOSED)
    {
        switch((msg->flags) & (SYN | ACK | FIN))
        {
            case SYN:
                if(socket->state == LISTEN)
                {
                    socket->state = SYN_RECEIVED;
                    socket->remotePort = Convert::ByteSwap(msg->srcPort);
                    socket->remoteIP = srcIP;
                    socket->acknowledgementNumber = Convert::ByteSwap( msg->sequenceNumber ) + 1;
                    socket->sequenceNumber = 0xbeefcafe;
                    Send(socket, 0,0, SYN|ACK);
                    socket->sequenceNumber++;
                }
                else
                    reset = true;
                break;
            case SYN | ACK:
                if(socket->state == SYN_SENT)
                {
                    socket->state = ESTABLISHED;
                    socket->acknowledgementNumber = Convert::ByteSwap( msg->sequenceNumber ) + 1;
                    socket->sequenceNumber++;
                    Send(socket, 0,0, ACK);
                }
                else
                    reset = true;
                break;
                
                
            case SYN | FIN:
            case SYN | FIN | ACK:
                reset = true;
                break;

                
            case FIN:
            case FIN|ACK:
                if(socket->state == ESTABLISHED)
                {
                    socket->state = CLOSE_WAIT;
                    socket->acknowledgementNumber++;
                    Send(socket, 0,0, ACK);
                    Send(socket, 0,0, FIN|ACK);
                }
                else if(socket->state == CLOSE_WAIT)
                {
                    socket->state = CLOSED;
                }
                else if(socket->state == FIN_WAIT1
                    || socket->state == FIN_WAIT2)
                {
                    socket->state = CLOSED;
                    socket->acknowledgementNumber++;
                    Send(socket, 0,0, ACK);
                }
                else
                    reset = true;
                break;
                
                
            case ACK:
                if(socket->state == SYN_RECEIVED)
                {
                    socket->state = ESTABLISHED;
                }
                else if(socket->state == FIN_WAIT1)
                {
                    socket->state = FIN_WAIT2;
                }
                else if(socket->state == CLOSE_WAIT)
                {
                    socket->state = CLOSED;
                    break;
                }
                
                if(msg->flags == ACK)
                    break;
                
            default:
                
                if(Convert::ByteSwap(msg->sequenceNumber) == socket->acknowledgementNumber)
                {
                    reset = !(socket->HandleTCPData(payload + msg->headerSize32*4,
                                                                              size - msg->headerSize32*4));
                    if(!reset)
                    {
                        int x = 0;
                        for(int i = msg->headerSize32*4; i < size; i++)
                            if(payload[i] != 0)
                                x = i;
                        socket->acknowledgementNumber += x - msg->headerSize32*4 + 1;
                        Send(socket, 0,0, ACK);
                    }
                }
                else
                {
                    // data in wrong order
                    reset = true;
                }
                
        }
    }
    
    
    if(reset)
    {
        if(socket != 0)
        {
            Send(socket, 0,0, RST);
        }
        else
        {
            TCPSocket socket(this);
            socket.remotePort = msg->srcPort;
            socket.remoteIP = srcIP;
            socket.localPort = msg->dstPort;
            socket.localIP = dstIP;
            socket.sequenceNumber = Convert::ByteSwap(msg->acknowledgementNumber);
            socket.acknowledgementNumber = Convert::ByteSwap(msg->sequenceNumber) + 1;
            Send(&socket, 0,0, RST);
        }
    }
    

    if(socket != 0 && socket->state == CLOSED)
        for(uint16_t i = 0; i < numSockets && socket == 0; i++)
            if(sockets[i] == socket)
            {
                sockets[i] = sockets[--numSockets];
                MemoryManager::activeMemoryManager->free(socket);
                break;
            }   
}

TCPSocket* TransmissionControlProtocolManager::Connect(common::uint32_t ip, common::uint16_t port)
{
    TCPSocket* socket = new TCPSocket(this);
    socket -> remotePort = port;
    socket -> remoteIP = ip;
    socket -> localPort = freePort++;
    socket -> localIP = backend->GetIPAddress();
    
    sockets[numSockets++] = socket;
    socket -> state = SYN_SENT;
    
    socket -> sequenceNumber = 0xbeefcafe;
        
    Send(socket, 0,0, SYN);
    return socket;
}
void TransmissionControlProtocolManager::Disconnect(TCPSocket* socket)
{
    socket->state = FIN_WAIT1;
    Send(socket, 0,0, FIN + ACK);
    socket->sequenceNumber++;
}
void TransmissionControlProtocolManager::Send(TCPSocket* socket, common::uint8_t* data, common::uint32_t size, common::uint16_t flags = 0)
{
    uint16_t totalLength = size + sizeof(TCPHeader);
    uint16_t lengthInclPHdr = totalLength + sizeof(TCPPseudoHeader);
    
    uint8_t* buffer = (uint8_t*)MemoryManager::activeMemoryManager->malloc(lengthInclPHdr);
    
    TCPPseudoHeader* phdr = (TCPPseudoHeader*)buffer;
    TCPHeader* msg = (TCPHeader*)(buffer + sizeof(TCPPseudoHeader));
    uint8_t* buffer2 = buffer + sizeof(TCPHeader)
                              + sizeof(TCPPseudoHeader);
    
    msg->headerSize32 = sizeof(TCPHeader)/4;
    msg->srcPort = Convert::ByteSwap(socket->localPort);
    msg->dstPort = Convert::ByteSwap(socket->remotePort);
    
    msg->acknowledgementNumber = Convert::ByteSwap(socket->acknowledgementNumber);
    msg->sequenceNumber = Convert::ByteSwap(socket->sequenceNumber);
    msg->reserved = 0;
    msg->flags = flags;
    msg->windowSize = 0xFFFF;
    msg->urgentPtr = 0;
    
    msg->options = ((flags & SYN) != 0) ? 0xB4050402 : 0;
    
    socket->sequenceNumber += size;
        
    for(int i = 0; i < size; i++)
        buffer2[i] = data[i];
    
    phdr->srcIP = socket->localIP;
    phdr->dstIP = socket->remoteIP;
    phdr->protocol = 0x0600;
    phdr->totalLength = ((totalLength & 0x00FF) << 8) | ((totalLength & 0xFF00) >> 8);    
    
    msg -> checksum = 0;
    msg -> checksum = IPV4Protocol::Checksum((uint16_t*)buffer, lengthInclPHdr);

    /*
    printf("Sending TCP:\n");
    printf("dstIP: "); NetTools::PrintIP(phdr->dstIP); printf("\n");
    printf("srcIP: "); NetTools::PrintIP(phdr->srcIP); printf("\n");
    */

    backend->ipv4->Send(socket->remoteIP, 0x06, (uint8_t*)msg, totalLength);
    MemoryManager::activeMemoryManager->free(buffer);   
}
TCPSocket* TransmissionControlProtocolManager::Listen(common::uint16_t port)
{
    TCPSocket* socket = new TCPSocket(this);
        
    socket -> state = LISTEN;
    socket -> localIP = backend->GetIPAddress();
    socket -> localPort = port;
        
    sockets[numSockets++] = socket;
    
    return socket;
}