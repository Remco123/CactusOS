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

void ReadyForRead()
{
    uint32_t timeOut = 100000;
    while(timeOut--)
        if((inportb(PS2_STATUS) & (1<<0)) == 1)
            return;
    
    Log(Warning, "Keyboard wait timeout");
}
bool ReadyForWrite()
{
    uint32_t timeOut = 100000;
    while(timeOut--)
        if((inportb(PS2_STATUS) & (1<<1)) == 0)
            return true;
    
    return false;
}

bool SendCommand(uint8_t cmd, uint8_t data = 0)
{
    if(ReadyForWrite() == false)
        return false;

    outportb(PS2_COMMAND, cmd);

    if(data) {
        if(ReadyForWrite() == false)
            return false;
        
        outportb(PS2_DATA, data);
    }
    return true;
}

bool PS2KeyboardDriver::Initialize()
{
    if(!SendCommand(0xAD)) // Send Disable keyboard command
        return false;
    
    if(!SendCommand(0x20)) // Indicate we would like to recieve the config byte
        return false;
    
    ReadyForRead();
    uint8_t status = (inportb(PS2_DATA) | 1) & ~0x10;
    if(!SendCommand(0x60, status)) // Update config byte
        return false;
    
    if(!SendCommand(0xAE)) // Send Enable keyboard command
        return false;
        
    // Add ourself to the list of known keyboards
    System::keyboardManager->keyboards.push_back(this);

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