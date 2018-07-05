#include <system/drivers/amd_am79c973.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);
void printfHex16(uint16_t);
void printfHex32(uint32_t);

amd_am79c973::amd_am79c973(PeripheralComponentInterconnectDeviceDescriptor *dev,
             InterruptManager* interrupts)
: NetworkDriver(dev, interrupts)
{
    currentSendBuffer = 0;
    currentRecvBuffer = 0;

    MAC[0] = inportw(dev->portBase) % 256;
    MAC[1] = inportw(dev->portBase) / 256;
    MAC[2] = inportw(dev->portBase + 0x02) % 256;
    MAC[3] = inportw(dev->portBase + 0x02) / 256;
    MAC[4] = inportw(dev->portBase + 0x04) % 256;
    MAC[5] = inportw(dev->portBase + 0x04) / 256;
    
    printf("PCNET MAC: "); 
    printfHex(MAC[0]); printf(":");
    printfHex(MAC[1]); printf(":");
    printfHex(MAC[2]); printf(":");
    printfHex(MAC[3]); printf(":");
    printfHex(MAC[4]); printf(":");
    printfHex(MAC[5]); printf("\n");

    uint64_t mac_addr = MAC[5] << 40
                 | MAC[4] << 32
                 | MAC[3] << 24
                 | MAC[2] << 16
                 | MAC[1] << 8
                 | MAC[0];

    printf("Resetting\n");
    Reset();

    // 32 bit mode
    outportw(device->portBase + 0x12, 20);
    outportw(device->portBase + 0x16, 0x102);
    
    // STOP reset
    outportw(device->portBase + 0x12, 0);
    outportw(device->portBase + 0x10, 0x04);

    initBlock.mode = 0x0000; // promiscuous mode = false
    initBlock.reserved1 = 0;
    initBlock.numSendBuffers = 3;
    initBlock.reserved2 = 0;
    initBlock.numRecvBuffers = 3;
    initBlock.physicalAddress = mac_addr;
    initBlock.reserved3 = 0;
    initBlock.logicalAddress = 0;
    
    sendBufferDescr = (BufferDescriptor*)((((uint32_t)&sendBufferDescrMemory[0]) + 15) & ~((uint32_t)0xF));
    initBlock.sendBufferDescrAddress = (uint32_t)sendBufferDescr;
    recvBufferDescr = (BufferDescriptor*)((((uint32_t)&recvBufferDescrMemory[0]) + 15) & ~((uint32_t)0xF));
    initBlock.recvBufferDescrAddress = (uint32_t)recvBufferDescr;

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

    outportw(device->portBase + 0x12, 1);
    outportw(device->portBase + 0x10,  (uint32_t)(&initBlock) & 0xFFFF );
    outportw(device->portBase + 0x12, 2);
    outportw(device->portBase + 0x10, ((uint32_t)(&initBlock) >> 16) & 0xFFFF );
    printf("PCNET Ready\n");
}
amd_am79c973::~amd_am79c973()
{

}

uint32_t amd_am79c973::HandleInterrupt(uint32_t esp)
{
    outportw(device->portBase + 0x12, 0);
    uint32_t temp = inportw(device->portBase + 0x10);
    
    if((temp & 0x8000) == 0x8000) printf("AMD am79c973 ERROR\n");
    if((temp & 0x2000) == 0x2000) printf("AMD am79c973 COLLISION ERROR\n");
    if((temp & 0x1000) == 0x1000) printf("AMD am79c973 MISSED FRAME\n");
    if((temp & 0x0800) == 0x0800) printf("AMD am79c973 MEMORY ERROR\n");
    if((temp & 0x0400) == 0x0400) HandleReceive();
    if((temp & 0x0200) == 0x0200) printf("Packet Send\n");
                               
    // acknoledge
    outportw(device->portBase + 0x12, 0);
    outportw(device->portBase + 0x10, temp);
    
    if((temp & 0x0100) == 0x0100) printf("AMD am79c973 INIT DONE\n");
    
    return esp;
}
void amd_am79c973::HandleReceive()
{
    printf("\nRECV: ");
    
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

            for(int i = 0; i < size; i++)
            {
                printfHex(buffer[i]);
                printf(" ");
            }

            //Handle packet here
            //HandlePacket(buffer, size)
        }
        
        recvBufferDescr[currentRecvBuffer].flags2 = 0;
        recvBufferDescr[currentRecvBuffer].flags = 0x8000F7FF;
    }
}
void amd_am79c973::SendData(uint8_t* buffer, uint32_t size)
{
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
void amd_am79c973::Activate()
{
    outportw(device->portBase + 0x12, 0);
    outportw(device->portBase + 0x10, 0x41);

    outportw(device->portBase + 0x12, 4);
    uint32_t temp = inportw(0x10);
    outportw(device->portBase + 0x12, 4);
    outportw(device->portBase + 0x10, temp | 0xC00);
    
    outportw(device->portBase + 0x12, 0);
    outportw(device->portBase + 0x10, 0x42);
}
void amd_am79c973::Reset()
{
    inportw(device->portBase + 0x14);
    outportw(device->portBase + 0x14, 0);
}