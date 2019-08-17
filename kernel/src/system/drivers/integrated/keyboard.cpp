#include <system/drivers/integrated/keyboard.h>
#include <core/port.h>
#include <system/bootconsole.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;
using namespace CactusOS::core;

/////////////////
// US/International keyboard map
/////////////////
char US_Keyboard[128] =
{
    0, 27,
    '1','2','3','4','5','6','7','8','9','0',
    '-','=','\b',
    '\t', /* tab */
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0, /* control */
    'a','s','d','f','g','h','j','k','l',';','\'', '`',
    0, /* left shift */
    '\\','z','x','c','v','b','n','m',',','.','/',
    0, /* right shift */
    '*',
    0, /* alt */
    ' ', /* space */
    0, /* caps lock */
    0, /* F1 [59] */
    0, 0, 0, 0, 0, 0, 0, 0,
    0, /* ... F10 */
    0, /* 69 num lock */
    0, /* scroll lock */
    0, /* home */
    0, /* up */
    0, /* page up */
    '-',
    0, /* left arrow */
    0,
    0, /* right arrow */
    '+',
    0, /* 79 end */
    0, /* down */
    0, /* page down */
    0, /* insert */
    0, /* delete */
    0, 0, 0,
    0, /* F11 */
    0, /* F12 */
    0, /* everything else */
};
char US_KeyboardShift[128] =
{
    0, 27,
    '!','@','#','$','%','^','&','*','(',')',
    '_','+','\b',
    '\t', /* tab */
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0, /* control */
    'A','S','D','F','G','H','J','K','L',':','"', '~',
    0, /* left shift */
    '|','Z','X','C','V','B','N','M','<','>','?',
    0, /* right shift */
    '*',
    0, /* alt */
    ' ', /* space */
    0, /* caps lock */
    0, /* F1 [59] */
    0, 0, 0, 0, 0, 0, 0, 0,
    0, /* ... F10 */
    0, /* 69 num lock */
    0, /* scroll lock */
    0, /* home */
    0, /* up */
    0, /* page up */
    '-',
    0, /* left arrow */
    0,
    0, /* right arrow */
    '+',
    0, /* 79 end */
    0, /* down */
    0, /* page down */
    0, /* insert */
    0, /* delete */
    0, 0, 0,
    0, /* F11 */
    0, /* F12 */
    0, /* everything else */
};

KeyboardDriver::KeyboardDriver()
: InterruptHandler(0x21), Driver("PS2 Keyboard", "Driver for a generic ps2 keyboard"), FIFOStream(100)
{
    status.Alt = false;
    status.CapsLock = false;
    status.Control = false;
    status.LeftShift = false;
    status.NumberLock = false;
    status.RightShift = false;

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

    if(key & 0x80)
    {
        key &= 0x7F;
        switch(key)
        {
            case LEFT_SHIFT:
                status.LeftShift = false;
                break;
            case RIGHT_SHIFT:
                status.RightShift = false;
                break;
        }
    }
    else
    {
        switch(key)
        {
            case CAPS_LOCK:
                status.CapsLock = !status.CapsLock;
                UpdateLeds();
                break;
            case NUM_LOCK:
                status.NumberLock = !status.NumberLock;
                UpdateLeds();
                break;
            case LEFT_SHIFT:
                status.LeftShift = true;
                break;
            case RIGHT_SHIFT:
                status.RightShift = true;
                break;
            default:
                char c = (status.LeftShift || status.RightShift) ? US_KeyboardShift[key] : US_Keyboard[key];
                if(System::screenMode == ScreenMode::GraphicsMode)
                    this->Write(c);
                //Log(Info, "Got key %c", c);
                break;
        }
    }

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