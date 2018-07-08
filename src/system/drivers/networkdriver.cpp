#include <system/drivers/networkdriver.h>

using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::core;

void printf(char*);
void printfHex(uint8_t);

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

uint64_t NetworkDriver::GetMacAddressBE()
{
    uint64_t result = ((uint64_t)MAC[0] << 40) |
                      ((uint64_t)MAC[1] << 32) |
                      ((uint64_t)MAC[2] << 24) |
                      ((uint64_t)MAC[3] << 16) |
                      ((uint64_t)MAC[4] << 8)  |
                      ((uint64_t)MAC[5] << 0);
    return result;
}