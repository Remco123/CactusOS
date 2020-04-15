#include <system/drivers/integrated/keyboard.h>
#include <core/port.h>
#include <system/bootconsole.h>
#include <system/system.h>
#include <../../lib/include/shared.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;
using namespace CactusOS::core;

KeyboardDriver::KeyboardDriver()
: InterruptHandler(0x21), Driver("PS2 Keyboard", "Driver for a generic ps2 keyboard"), FIFOStream(100)
{
    status.LeftShift = false;
    status.RightShift = false;
    status.Alt = false;
    status.LeftControl = false;
    status.RightControl = false;
    status.CapsLock = false;
    status.NumberLock = false;

    System::keyboardStream = this;
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
     
    LIBCactusOS::KeypressPacket packet = {.startByte = KEYPACKET_START, .keyCode = 0, .flags = LIBCactusOS::NoFlags};

    if(!(key & 0x80))   // Pressed
        packet.flags = packet.flags | LIBCactusOS::Pressed;
    else                // Released
        key &= 0x7F;    // Remove first bit indicating state
    
    if(key == 0x7A) // CapsLock also sends 2 0x7A keycodes for some reason
        return esp;
    
    ////////////
    // Update driver status Flags
    ////////////
    if(key == 0x3A && !(packet.flags & LIBCactusOS::Pressed)) { //Toggle Keys
        status.CapsLock = !status.CapsLock;
        UpdateLeds();
    }
    else if(key == 0x45 && !(packet.flags & LIBCactusOS::Pressed)) { //Toggle Keys
        status.NumberLock = !status.NumberLock;
        UpdateLeds();
    }
    //Modifier Keys
    else if(key == 0x2A)
        status.LeftShift = packet.flags & LIBCactusOS::Pressed;
    else if(key == 0x36)
        status.RightShift = packet.flags & LIBCactusOS::Pressed;
    else if(key == 0x1D)
        status.LeftControl = packet.flags & LIBCactusOS::Pressed;
    else if(key == 0xE0)
        status.RightControl = packet.flags & LIBCactusOS::Pressed;
    else if(key == 0x38)
        status.Alt = packet.flags & LIBCactusOS::Pressed;
    
    // Update packet flags
    packet.flags = (packet.flags | (status.CapsLock ? LIBCactusOS::CapsLock : LIBCactusOS::NoFlags));
    packet.flags = (packet.flags | (status.NumberLock ? LIBCactusOS::NumLock : LIBCactusOS::NoFlags));
    packet.flags = (packet.flags | (status.LeftShift ? LIBCactusOS::LeftShift : LIBCactusOS::NoFlags));
    packet.flags = (packet.flags | (status.RightShift ? LIBCactusOS::RightShift : LIBCactusOS::NoFlags));
    packet.flags = (packet.flags | (status.LeftControl ? LIBCactusOS::LeftControl : LIBCactusOS::NoFlags));
    packet.flags = (packet.flags | (status.RightControl ? LIBCactusOS::RightControl : LIBCactusOS::NoFlags));
    packet.flags = (packet.flags | (status.Alt ? LIBCactusOS::Alt : LIBCactusOS::NoFlags));

    // Set keycode
    packet.keyCode = key;

    //Log(Info, "%d %d %d %d %d %d %d", status.CapsLock,status.NumberLock,status.LeftShift,status.RightShift,status.LeftControl,status.RightControl,status.Alt);

    if(System::screenMode == ScreenMode::GraphicsMode)
        for(int i = 0; i < sizeof(LIBCactusOS::KeypressPacket); i++)
            this->Write(*((char*)((uint32_t)&packet + i)));
    else if(System::setupMode == true && (packet.flags & LIBCactusOS::Pressed))
        this->Write(key); //Make things easier for the setup

    return esp;
}

void KeyboardDriver::UpdateLeds()
{
	uint8_t code = 0;

	if(status.NumberLock)
		code |= 1 << 1;
		
	if(status.CapsLock)
		code |= 1 << 2;

    while((inportb(0x64) & 2) != 0);
    outportb(0x60, 0xED);
    while((inportb(0x64) & 2) != 0);
    outportb(0x60, code);
}