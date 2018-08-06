#include <system/drivers/keyboard.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);

KeyboardDriver::KeyboardDriver(core::InterruptManager* interrupts)
: InterruptHandler(interrupts, 0x21)
{
    this->Type = DriverType::Keyboard;
}
KeyboardDriver::~KeyboardDriver()
{

}

uint32_t KeyboardDriver::HandleInterrupt(uint32_t esp)
{
    uint8_t key = inportb(0x60);
    
    if(key < 80)
    {
        this->keyAvailibe = true;
        switch(key)
        {
            case 0x02: this->lastKey = '1'; break;
            case 0x03: this->lastKey = '2'; break;
            case 0x04: this->lastKey = '3'; break;
            case 0x05: this->lastKey = '4'; break;
            case 0x06: this->lastKey = '5'; break;
            case 0x07: this->lastKey = '6'; break;
            case 0x08: this->lastKey = '7'; break;
            case 0x09: this->lastKey = '8'; break;
            case 0x0A: this->lastKey = '9'; break;
            case 0x0B: this->lastKey = '0'; break;

            case 0x10: this->lastKey = 'q'; break;
            case 0x11: this->lastKey = 'w'; break;
            case 0x12: this->lastKey = 'e'; break;
            case 0x13: this->lastKey = 'r'; break;
            case 0x14: this->lastKey = 't'; break;
            case 0x15: this->lastKey = 'z'; break;
            case 0x16: this->lastKey = 'u'; break;
            case 0x17: this->lastKey = 'i'; break;
            case 0x18: this->lastKey = 'o'; break;
            case 0x19: this->lastKey = 'p'; break;

            case 0x1E: this->lastKey = 'a'; break;
            case 0x1F: this->lastKey = 's'; break;
            case 0x20: this->lastKey = 'd'; break;
            case 0x21: this->lastKey = 'f'; break;
            case 0x22: this->lastKey = 'g'; break;
            case 0x23: this->lastKey = 'h'; break;
            case 0x24: this->lastKey = 'j'; break;
            case 0x25: this->lastKey = 'k'; break;
            case 0x26: this->lastKey = 'l'; break;

            case 0x2C: this->lastKey = 'y'; break;
            case 0x2D: this->lastKey = 'x'; break;
            case 0x2E: this->lastKey = 'c'; break;
            case 0x2F: this->lastKey = 'v'; break;
            case 0x30: this->lastKey = 'b'; break;
            case 0x31: this->lastKey = 'n'; break;
            case 0x32: this->lastKey = 'm'; break;
            case 0x33: this->lastKey = ','; break;
            case 0x34: this->lastKey = '.'; break;
            case 0x35: this->lastKey = '-'; break;

            case 0x1C: this->lastKey = '\n'; break;
            case 0x39: this->lastKey = ' '; break;
            case 0x0E: this->lastKey = '\b'; break;

            default:
                this->keyAvailibe = false; //no key if we reach this
                break;
        }
    }


    return esp;
}
void KeyboardDriver::Activate()
{
    while(inportb(0x64) & 0x1)
        inportb(0x60);
    outportb(0x64, 0xae);
    outportb(0x64, 0x20);
    uint8_t status = (inportb(0x60) | 1) & ~0x10;
    outportb(0x64, 0x60);
    outportb(0x60, status);
    outportb(0x60, 0xf4);
}

char KeyboardDriver::GetKey(bool wait)
{
    if(wait)
        while(!keyAvailibe);
    keyAvailibe = false;
    return lastKey;
}