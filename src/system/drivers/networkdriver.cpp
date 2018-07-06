#include <system/drivers/networkdriver.h>

using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::core;

void printf(char*);

NetworkDriver::NetworkDriver(PeripheralComponentInterconnectDeviceDescriptor* dev, InterruptManager* interrupts)
: Driver(), 
  InterruptHandler(interrupts, interrupts->HardwareInterruptOffset() + dev->interrupt)
{
    this->device = dev;
    this->Type = DriverType::Network;
}

NetworkDriver::~NetworkDriver()
{

}


uint32_t NetworkDriver::HandleInterrupt(uint32_t esp)
{
    printf("Warning! NetworkDriver::HandleInterrupt() Should not be called\n");
    return esp;
}
void NetworkDriver::HandleReceive()
{
    printf("Warning! NetworkDriver::HandleReceive() Should not be called\n");
}
void NetworkDriver::SendData(uint8_t* data, uint32_t datalen)
{
    printf("Warning! NetworkDriver::SendData(uint8_t*,uint32_t) Should not be called\n");
}
void NetworkDriver::Activate()
{
    printf("Warning! NetworkDriver::Activate() Should not be called\n");
}
void NetworkDriver::Reset()
{
    printf("Warning! NetworkDriver::Reset() Should not be called\n");
}
uint64_t NetworkDriver::GetMacAddress()
{
    uint64_t mac_addr = MAC[5] << 40
                 | MAC[4] << 32
                 | MAC[3] << 24
                 | MAC[2] << 16
                 | MAC[1] << 8
                 | MAC[0];

    return mac_addr;
}