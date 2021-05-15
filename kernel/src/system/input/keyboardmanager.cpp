#include <system/input/keyboardmanager.h>
#include <../../lib/include/shared.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;

KeyboardManager::KeyboardManager()
{ 
    MemoryOperations::memset(&this->sharedStatus, 0, sizeof(this->sharedStatus));
}

void KeyboardManager::HandleKeyChange(Keyboard* src, uint32_t key, bool pressed)
{
    if(src->type == KeyboardType::USB)
        key = ConvertToPS2(key);

    //Log(Info, "Got key %d from keyboard, pressed = %b", key, pressed);
    
    bool updateLeds = false;
    LIBCactusOS::KeypressPacket packet = {.startByte = KEYPACKET_START, .keyCode = 0, .flags = pressed ? LIBCactusOS::Pressed : LIBCactusOS::NoFlags};

    ////////////
    // Update keyboards globals
    ////////////
    if(key == 0x3A && pressed) { // Toggle Keys
        this->sharedStatus.CapsLock = !this->sharedStatus.CapsLock;
        updateLeds = true;
    }
    else if(key == 0x45 && pressed) { //Toggle Keys
        this->sharedStatus.NumLock = !this->sharedStatus.NumLock;
        updateLeds = true;
    }
    
    // Update packet flags
    packet.flags = (packet.flags | (this->sharedStatus.CapsLock ? LIBCactusOS::CapsLock : LIBCactusOS::NoFlags));
    packet.flags = (packet.flags | (this->sharedStatus.NumLock ? LIBCactusOS::NumLock : LIBCactusOS::NoFlags));
    
    packet.flags = (packet.flags | (src->status.LeftShift ? LIBCactusOS::LeftShift : LIBCactusOS::NoFlags));
    packet.flags = (packet.flags | (src->status.RightShift ? LIBCactusOS::RightShift : LIBCactusOS::NoFlags));
    packet.flags = (packet.flags | (src->status.LeftControl ? LIBCactusOS::LeftControl : LIBCactusOS::NoFlags));
    packet.flags = (packet.flags | (src->status.RightControl ? LIBCactusOS::RightControl : LIBCactusOS::NoFlags));
    packet.flags = (packet.flags | (src->status.Alt ? LIBCactusOS::Alt : LIBCactusOS::NoFlags));

    // Set keycode
    packet.keyCode = key;

    if(System::screenMode == ScreenMode::GraphicsMode)
        for(int i = 0; i < (int)sizeof(LIBCactusOS::KeypressPacket); i++)
            this->Write(*((char*)((uint32_t)&packet + i)));
    else if(System::setupMode == true && (packet.flags & LIBCactusOS::Pressed))
        this->Write(key); //Make things easier for the setup

    if(updateLeds) {
        for(Keyboard* kbd : this->keyboards)
            if(kbd->type != USB) // USB keyboards need interrupts enabled. TODO: Change this perhaps?
                kbd->UpdateLEDS();
    }
}
uint8_t KeyboardManager::ConvertToPS2(uint32_t key)
{
    if(key < 4)
        return 0; // Invallid

    switch(key)
    {
        case 0x4: return 0x1E;
        case 0x5: return 0x30;
        case 0x6: return 0x2E;
        case 0x7: return 0x20;
        case 0x8: return 0x12;
        case 0x9: return 0x21;
        case 0xA: return 0x22;
        case 0xB: return 0x23;
        case 0xC: return 0x17;
        case 0xD: return 0x24;
        case 0xE: return 0x25;
        case 0xF: return 0x26;
        case 0x10: return 0x32;
        case 0x11: return 0x31;
        case 0x12: return 0x18;
        case 0x13: return 0x19;
        case 0x14: return 0x10;
        case 0x15: return 0x13;
        case 0x16: return 0x1F;
        case 0x17: return 0x14;
        case 0x18: return 0x16;
        case 0x19: return 0x2F;
        case 0x1A: return 0x11;
        case 0x1B: return 0x2D;
        case 0x1C: return 0x15;
        case 0x1D: return 0x2C;
        case 0x1E ... 0x26: return (key - 0x1E + 2);
        case 0x27: return 0xB;
        case 0x28: return 0x1C;
        case 0x29: return 0x1;
        case 0x2A: return 0xE;
        case 0x2B: return 0xF;
        case 0x2C: return 0x39;
        case 0x2D: return 0xC;
        case 0x2E: return 0xD;
        case 0x2F: return 0x1A;
        case 0x30: return 0x1B;
        case 0x31: return 0x2B;
        case 0x32: return 0x2B;
        case 0x33: return 0x27;
        case 0x34: return 0x28;
        case 0x35: return 0x29;
        case 0x36: return 0x33;
        case 0x37: return 0x34;
        case 0x38: return 0x35;
        case 0x39: return 0x3A;
        case 0x3A ... 0x43: return key + 1;
        case 0x44: return 0x57;
        case 0x45: return 0x58;
        case 0x49: return 0x52;
        case 0x4A: return 0x47;
        case 0x4B: return 0x49;
        case 0x4C: return 0x53;
        case 0x4D: return 0x4F;
        case 0x4E: return 0x51;
        case 0x4F: return 0x4D;
        case 0x50: return 0x4B;
        case 0x51: return 0x50;
        case 0x52: return 0x48;
        case 0x53: return 0x45;
        case 0x54: return 0x35;
        case 0x55: return 0x37;
        case 0x56: return 0x4A;
        case 0x57: return 0x4E;
        case 0x58: return 0x1C;
        case 0x59: return 0x4F;
        case 0x5A: return 0x50;
        case 0x5B: return 0x51;
        case 0x5C: return 0x4B;
        case 0x5D: return 0x4C;
        case 0x5E: return 0x4D;
        case 0x5F: return 0x47;
        case 0x60: return 0x48;
        case 0x61: return 0x49;
        case 0x62: return 0x52;
        case 0x63: return 0x53;

        default: return 0;
    }
}