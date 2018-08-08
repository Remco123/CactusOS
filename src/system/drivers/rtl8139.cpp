#include <system/drivers/rtl8139.h>

using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);
void printfHex16(uint16_t);
void printfHex32(uint32_t);

uint32_t current_packet_ptr;

// Four TXAD register, you must use a different one to send packet each time(for example, use the first one, second... fourth and back to the first)
uint8_t TSAD_array[4] = {0x20, 0x24, 0x28, 0x2C};
uint8_t TSD_array[4] = {0x10, 0x14, 0x18, 0x1C};

RTL8139::RTL8139(PeripheralComponentInterconnectDeviceDescriptor *dev, InterruptManager* interrupts, PeripheralComponentInterconnectController* pci)
: NetworkDriver(dev, interrupts) //Remember the correct interrupt is also set by the networkdriver!
{
    //Read portbase of device
    device->portBase = pci->Read(device->bus, device->device, device->function, 0x10);

    // Enable PCI Bus Mastering
    uint32_t pci_command_reg = pci->Read(device->bus, device->device, device->function, 0x04);
    if(!(pci_command_reg & (1 << 2))) {
        pci_command_reg |= (1 << 2);
        pci->Write(device->bus, device->device, device->function, 0x04, pci_command_reg);
    }

    printf("Interrupt: "); printfHex(device->interrupt); printf("\n");
    printf("PortBase: "); printfHex32(device->portBase); printf("\n");

    if( (device->portBase & 0x1) == 1)
    {
        printf("Detected I/O type \n");
        device->portBase = device->portBase & 0xfffffffc ;
        printf("New PortBase: "); printfHex32(device->portBase); printf("\n");
    }
    else
    {
        printf("Detected unknown mapped type!\n");
        return;
    }

    //read mac
    printf("Card Mac Address = ");
    for(unsigned int i=0; i<6; i++)
    {
        this->MAC[i] = (uint8_t) inportb(device->portBase + i);
        printfHex(this->MAC[i]); printf(" ");
    }
    printf("\n");
}
RTL8139::~RTL8139()
{

}

uint32_t RTL8139::HandleInterrupt(uint32_t esp)
{
    printf("Got Interrupt ");
    
    uint16_t status = inportw(device->portBase + 0x3e);

    printfHex16(status); printf(" From RTL8139\n");

    if(status & TOK) {
        printf("Packet sent\n");
    }
    if (status & ROK) {
        printf("RTL8139 Received packet\n");
        HandleReceive();
    }

    outportw(device->portBase + 0x3E, 0x5);
    return esp;
}
void RTL8139::HandleReceive()
{
    uint16_t * t = (uint16_t*)(rx_buffer + current_packet_ptr);
    // Skip packet header, get packet length
    uint16_t packet_length = *(t + 1);

    // Skip, packet header and packet length, now t points to the packet data
    t = t + 2;

    uint8_t* packet = (uint8_t*)MemoryManager::activeMemoryManager->malloc(packet_length);
    MemoryOperations::memcpy(packet, t, packet_length);
    if(this->NetManager != 0)
        this->NetManager->HandlePacket(packet, packet_length);
    else
        printf("RTL8139: Could not pass to handler\n");

    current_packet_ptr = (current_packet_ptr + packet_length + 4 + 3) & RX_READ_POINTER_MASK;

    if(current_packet_ptr > RX_BUF_SIZE)
        current_packet_ptr -= RX_BUF_SIZE;

    outportw(device->portBase + CAPR, current_packet_ptr - 0x10);
}
void RTL8139::SendData(uint8_t* data, uint32_t len)
{
    void* transfer_data = MemoryManager::activeMemoryManager->malloc(len);
    MemoryOperations::memcpy(transfer_data, data, len);

    // Second, fill in physical address of data, and length
    outportl(device->portBase + TSAD_array[tx_cur], (uint32_t)transfer_data);
    outportl(device->portBase + TSD_array[tx_cur++], len);
    if(tx_cur > 3)
        tx_cur = 0;
}
void RTL8139::Activate()
{
    printf("Activating RTL8139\n");
    outportb(device->portBase + 0x52, 0x0);

    Reset();

    rx_buffer = (char*) MemoryManager::activeMemoryManager->malloc (8192 + 16 + 1500);
    MemoryOperations::memset(rx_buffer, 0x0, 8192 + 16 + 1500);
    outportl(device->portBase + 0x30, (uint32_t)rx_buffer);

    // Sets the TOK and ROK bits high
    outportw(device->portBase + 0x3C, 0x0005);

    // Sets the RE and TE bits high
    outportb(device->portBase + 0x37, 0x0C);

    // (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP
    outportl(device->portBase + 0x44, 0xF);
}
void RTL8139::Reset()
{
    printf("Resetting\n");
    outportb(device->portBase + 0x37, 0x10);
    while((inportb(device->portBase + 0x37) & 0x10) != 0) {
        printf(".");
    }
}