#include <system/components/rtc.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

bool RTC::UpdateInProgress()
{
    outportb(0x70, 0x0A);
    return (inportb(0x71) & 0x80);
}
uint8_t RTC::ReadRegister(int reg)
{
    outportb(0x70, reg);
    return inportb(0x71);
}

RTC::RTC()
: SystemComponent("RTC", "Legacy Real Time Clock")
{   }

uint32_t RTC::GetSecond()
{
    while(UpdateInProgress());
    uint32_t value = ReadRegister(0x00);
    uint32_t registerB = ReadRegister(0x0B);

    if(!(registerB & 0x04))
        value = (value & 0x0F) + ((value / 16) * 10);
    return value;
}
uint32_t RTC::GetMinute()
{
    while(UpdateInProgress());
    uint32_t value = ReadRegister(0x02);
    uint32_t registerB = ReadRegister(0x0B);

    if(!(registerB & 0x04))
        value = (value & 0x0F) + ((value / 16) * 10);
    return value;
}
uint32_t RTC::GetHour()
{
    while(UpdateInProgress());
    uint32_t value = ReadRegister(0x04);
    uint32_t registerB = ReadRegister(0x0B);

    if(!(registerB & 0x04))
        value = ((value & 0x0F) + (((value & 0x70) / 16) * 10) ) | (value & 0x80);
    
    if(!(registerB & 0x02) && (value & 0x80))
        value = ((value & 0x7F) + 12) % 24;

    return value;
}
uint32_t RTC::GetDay()
{
    while(UpdateInProgress());
    uint32_t value = ReadRegister(0x07);
    uint32_t registerB = ReadRegister(0x0B);

    if(!(registerB & 0x04))
        value = (value & 0x0F) + ((value / 16) * 10);
    return value;
}
uint32_t RTC::GetMonth()
{
    while(UpdateInProgress());
    uint32_t value = ReadRegister(0x08);
    uint32_t registerB = ReadRegister(0x0B);

    if(!(registerB & 0x04))
        value = (value & 0x0F) + ((value / 16) * 10);
    return value;
}
uint32_t RTC::GetYear()
{
    while(UpdateInProgress());
    uint32_t value = ReadRegister(0x09);
    uint32_t registerB = ReadRegister(0x0B);

    if(!(registerB & 0x04))
        value = (value & 0x0F) + ((value / 16) * 10);
    
    // Calculate the full (4-digit) year
    value += (CURRENT_YEAR / 100) * 100;
    if(value < CURRENT_YEAR)
        value += 100;

    return value;
}