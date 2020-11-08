#include <system/input/keyboard.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;

Keyboard::Keyboard(KeyboardType type)
{
    this->type = type;
    MemoryOperations::memset(&this->status, 0, sizeof(this->status));
}

void Keyboard::UpdateLEDS()
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
}

bool Keyboard::ContainsKey(uint8_t key, uint8_t* packet, int* pos)
{
    for(int i = 2; i < 8; i++)
        if(packet[i] == key) { *pos = i; return true; }
    return false;
}