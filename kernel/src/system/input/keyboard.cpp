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