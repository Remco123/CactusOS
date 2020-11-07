#include <system/drivers/integrated/ps2-keyboard.h>
#include <core/port.h>
#include <system/bootconsole.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;
using namespace CactusOS::core;

PS2KeyboardDriver::PS2KeyboardDriver()
: InterruptHandler(0x21), Keyboard(KeyboardType::PS2), Driver("PS2 Keyboard", "Driver for a generic ps2 keyboard"), FIFOStream(100)
{ }

bool PS2KeyboardDriver::Initialize()
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
uint32_t PS2KeyboardDriver::HandleInterrupt(uint32_t esp)
{
    uint8_t key = inportb(0x60);
    bool pressed = !(key & (1<<7));

    // Remove first bit from key
    if(!pressed)
        key = key & 0x7F;
    
    if(key == 0x7A) // CapsLock also sends 2 0x7A keycodes for some reason
        return esp;
     
    // Update internal modifier keys
    if(key == 0x2A)
        status.LeftShift = pressed;
    else if(key == 0x36)
        status.RightShift = pressed;
    else if(key == 0x1D)
        status.LeftControl = pressed;
    else if(key == 0xE0)
        status.RightControl = pressed;
    else if(key == 0x38)
        status.Alt = pressed;

    System::keyboardManager->HandleKeyChange(this, key, pressed);

    return esp;
}

void PS2KeyboardDriver::UpdateLEDS()
{
	uint8_t code = 0;

	if(System::keyboardManager->sharedStatus.NumLock)
		code |= 1 << 1;
		
	if(System::keyboardManager->sharedStatus.CapsLock)
		code |= 1 << 2;

    while((inportb(0x64) & 2) != 0);
    outportb(0x60, 0xED);
    while((inportb(0x64) & 2) != 0);
    outportb(0x60, code);
}