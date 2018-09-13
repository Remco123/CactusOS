#include <system/drivers/pcnet.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);
void printfHex32(uint32_t);

PCNET::PCNET(PeripheralComponentInterconnectDeviceDescriptor *dev,InterruptManager* interrupts, PeripheralComponentInterconnectController* pci)
: NetworkDriver(dev, interrupts) //Remember the correct interrupt is also set by the networkdriver!
{
    this->device = dev;
    currentSendBuffer = 0;
    currentRecvBuffer = 0;
    
    uint64_t MAC0 = inportw(dev->portBase) % 256;
    uint64_t MAC1 = inportw(dev->portBase) / 256;
    uint64_t MAC2 = inportw(dev->portBase + 0x02) % 256;
    uint64_t MAC3 = inportw(dev->portBase + 0x02) / 256;
    uint64_t MAC4 = inportw(dev->portBase + 0x04) % 256;
    uint64_t MAC5 = inportw(dev->portBase + 0x04) / 256;
    
    uint64_t MAC = MAC5 << 40
                 | MAC4 << 32
                 | MAC3 << 24
                 | MAC2 << 16
                 | MAC1 << 8
                 | MAC0;

    this->MAC[0] = MAC0;
    this->MAC[1] = MAC1;
    this->MAC[2] = MAC2;
    this->MAC[3] = MAC3;
    this->MAC[4] = MAC4;
    this->MAC[5] = MAC5;
    
    // 32 bit mode
    outportw(dev->portBase + 0x12, 20);
    outportw(dev->portBase + 0x16, 0x102);
    
    // STOP reset
    outportw(dev->portBase + 0x12, 0);
    outportw(dev->portBase + 0x10, 0x04);

    InitializationBlock* initBlock = new InitializationBlock();
    
    // initBlock
    initBlock->mode = 0x0000; // promiscuous mode = false
    initBlock->reserved1 = 0;
    initBlock->numSendBuffers = 3;
    initBlock->reserved2 = 0;
    initBlock->numRecvBuffers = 3;
    initBlock->physicalAddress = MAC;
    initBlock->reserved3 = 0;
    initBlock->logicalAddress = 0;
    
    sendBufferDescr = (BufferDescriptor*)((((uint32_t)&sendBufferDescrMemory[0]) + 15) & ~((uint32_t)0xF));
    initBlock->sendBufferDescrAddress = (uint32_t)sendBufferDescr;
    recvBufferDescr = (BufferDescriptor*)((((uint32_t)&recvBufferDescrMemory[0]) + 15) & ~((uint32_t)0xF));
    initBlock->recvBufferDescrAddress = (uint32_t)recvBufferDescr;
    
    for(uint8_t i = 0; i < 8; i++)
    {
        sendBufferDescr[i].address = (((uint32_t)&sendBuffers[i]) + 15 ) & ~(uint32_t)0xF;
        sendBufferDescr[i].flags = 0x7FF
                                 | 0xF000;
        sendBufferDescr[i].flags2 = 0;
        sendBufferDescr[i].avail = 0;
        
        recvBufferDescr[i].address = (((uint32_t)&recvBuffers[i]) + 15 ) & ~(uint32_t)0xF;
        recvBufferDescr[i].flags = 0xF7FF
                                 | 0x80000000;
        recvBufferDescr[i].flags2 = 0;
        sendBufferDescr[i].avail = 0;
    }
    
    outportw(dev->portBase + 0x12, 1);
    outportw(dev->portBase + 0x10,  (uint32_t)initBlock & 0xFFFF );
    outportw(dev->portBase + 0x12, 2);
    outportw(dev->portBase + 0x10, ((uint32_t)initBlock >> 16) & 0xFFFF );
}
PCNET::~PCNET()
{

}
            
uint32_t PCNET::HandleInterrupt(uint32_t esp)
{
    outportw(device->portBase + 0x12, 0);
    uint32_t temp = inportw(device->portBase + 0x10);

    //printf("AMD-PCnet Interrupt: "); printfHex32(temp); printf("\n");
    
    if((temp & 0x8000) == 0x8000) printf("AMD am79c973 ERROR\n");
    if((temp & 0x2000) == 0x2000) printf("AMD am79c973 COLLISION ERROR\n");
    if((temp & 0x1000) == 0x1000) printf("AMD am79c973 MISSED FRAME\n");
    if((temp & 0x0800) == 0x0800) printf("AMD am79c973 MEMORY ERROR\n");
    if((temp & 0x0400) == 0x0400) HandleReceive();
    if((temp & 0x0200) == 0x0200) printf(" AMD-PCNet packet send\n");
                               
    // acknoledge
    outportw(device->portBase + 0x12, 0);
    outportw(device->portBase + 0x10, temp);
    
    if((temp & 0x0100) == 0x0100) printf("AMD am79c973 INIT DONE\n");
    
    return esp;
}
void PCNET::HandleReceive()
{
    for(; (recvBufferDescr[currentRecvBuffer].flags & 0x80000000) == 0;
        currentRecvBuffer = (currentRecvBuffer + 1) % 8)
    {
        if(!(recvBufferDescr[currentRecvBuffer].flags & 0x40000000)
         && (recvBufferDescr[currentRecvBuffer].flags & 0x03000000) == 0x03000000) 
        
        {
            uint32_t size = recvBufferDescr[currentRecvBuffer].flags & 0xFFF;
            if(size > 64) // remove checksum
                size -= 4;
            
            uint8_t* buffer = (uint8_t*)(recvBufferDescr[currentRecvBuffer].address);
            /*
            if(this->NetManager != 0)
                this->NetManager->HandlePacket(buffer, size);
            */
        }
        
        recvBufferDescr[currentRecvBuffer].flags2 = 0;
        recvBufferDescr[currentRecvBuffer].flags = 0x8000F7FF;
    }
}
void PCNET::SendData(uint8_t* buffer, uint32_t size)
{
    printf("Sending...");
    int sendDescriptor = currentSendBuffer;
    currentSendBuffer = (currentSendBuffer + 1) % 8;
    
    if(size > 1518)
        size = 1518;
    
    for(uint8_t *src = buffer + size -1,
                *dst = (uint8_t*)(sendBufferDescr[sendDescriptor].address + size -1);
                src >= buffer; src--, dst--)
        *dst = *src;
    
    sendBufferDescr[sendDescriptor].avail = 0;
    sendBufferDescr[sendDescriptor].flags2 = 0;
    sendBufferDescr[sendDescriptor].flags = 0x8300F000
                                          | ((uint16_t)((-size) & 0xFFF));
    outportw(device->portBase + 0x12, 0);
    outportw(device->portBase + 0x10, 0x48);
}
void PCNET::Activate()
{
    printf("Starting AMD-PCNet\n");
    outportw(device->portBase + 0x12, 0);
    outportw(device->portBase + 0x10, 0x41);

    outportw(device->portBase + 0x12, 4);
    uint32_t temp = inportw(device->portBase + 0x10);
    outportw(device->portBase + 0x12, 4);
    outportw(device->portBase + 0x10, temp | 0xC00);
    
    outportw(device->portBase + 0x12, 0);
    outportw(device->portBase + 0x10, 0x42);

    printf("Card Mac Address = ");
    for(int i = 0; i < 6; i++)
        printfHex(this->MAC[i]);
    printf("\n");
    printf("Activate Done\n");
}
void PCNET::Reset()
{
    inportw(device->portBase + 0x14);
    outportw(device->portBase + 0x14, 0);
}