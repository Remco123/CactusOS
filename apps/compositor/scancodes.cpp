#include <types.h>
#include <shared.h>

using namespace LIBCactusOS;

/////////////////
// US/International keyboard map
/////////////////
const uint8_t US_Keyboard[128] =
{
    0, EscapeKey,
    '1','2','3','4','5','6','7','8','9','0',
    '-','=','\b',
    '\t', /* tab */
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',
    LeftControlKey, /* control */
    'a','s','d','f','g','h','j','k','l',';','\'', '\\',
    LeftShiftKey, /* left shift */
    '\\','z','x','c','v','b','n','m',',','.','/',
    RightShiftKey, /* right shift */
    '*',
    AltKey, /* alt */
    ' ', /* space */
    CapsLockKey, /* caps lock */
    F1Key, /* F1 [59] */
    F2Key, F3Key, F4Key, F5Key, F6Key, F7Key, F8Key, F9Key,
    F10Key, /* ... F10 */
    NumLockKey, /* 69 num lock */
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
    F11Key, /* F11 */
    F12Key, /* F12 */
    0, /* everything else */
};
const uint8_t US_KeyboardShift[128] =
{
    0, EscapeKey,
    '!','@','#','$','%','^','&','*','(',')',
    '_','+','\b',
    '\t', /* tab */
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    LeftControlKey, /* control */
    'A','S','D','F','G','H','J','K','L',':','\"', '|',
    LeftShiftKey, /* left shift */
    '|','Z','X','C','V','B','N','M','<','>','?',
    RightShiftKey, /* right shift */
    '*',
    AltKey, /* alt */
    ' ', /* space */
    CapsLockKey, /* caps lock */
    F1Key, /* F1 [59] */
    F2Key, F3Key, F4Key, F5Key, F6Key, F7Key, F8Key, F9Key,
    F10Key, /* ... F10 */
    NumLockKey, /* 69 num lock */
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
    F11Key, /* F11 */
    F12Key, /* F12 */
    0, /* everything else */
};

uint8_t ConvertKeycode(KeypressPacket* packet)
{
    bool upperCase = (packet->flags & KEYPACKET_FLAGS::LeftShift) || (packet->flags & KEYPACKET_FLAGS::RightShift) || (packet->flags & KEYPACKET_FLAGS::CapsLock);
    if(upperCase)
        return US_KeyboardShift[packet->keyCode];
    else
        return US_Keyboard[packet->keyCode];
}