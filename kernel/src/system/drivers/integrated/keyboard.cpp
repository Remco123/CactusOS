#include <system/drivers/integrated/keyboard.h>
#include <core/port.h>
#include <system/bootconsole.h>
#include <system/log.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;
using namespace CactusOS::core;

KeyboardDriver::KeyboardDriver()
: InterruptHandler(0x21), Driver("PS2 Keyboard", "Driver for a generic ps2 keyboard")
{

}

bool KeyboardDriver::Initialize()
{
    while(inportb(0x64) & 0x1)
        inportb(0x60);

    outportb(0x64, 0xae);
    outportb(0x64, 0x20);
    uint8_t status = (inportb(0x60) | 1) & ~0x10;
    outportb(0x64, 0x60);
    outportb(0x60, status);
    outportb(0x60, 0xf4);

    return true;
}
uint32_t KeyboardDriver::HandleInterrupt(uint32_t esp)
{
    uint8_t key = inportb(0x60);

    Log(Info, "Got key: %b", key);

    return esp;
}