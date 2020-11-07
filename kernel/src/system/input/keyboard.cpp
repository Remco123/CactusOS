#include <system/input/keyboard.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;

Keyboard::Keyboard(KeyboardType type)
{
    this->type = type;
    MemoryOperations::memset(&this->status, 0, sizeof(InternalKeyboardStatus));
    
    // Add ourself to the list of known keyboards
    System::keyboardManager->keyboards.push_back(this);
}

Keyboard::~Keyboard()
{
    // Remove ourself from the list of known keyboards
    System::keyboardManager->keyboards.Remove(this);
}

void Keyboard::UpdateLEDS()
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
}