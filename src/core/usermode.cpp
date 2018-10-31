#include <core/usermode.h>

using namespace CactusOS::core;
using namespace CactusOS;

extern "C" void enter_user_mode();

void Usermode::EnterUserMode()
{
    enter_user_mode(); //call the assembly function
}