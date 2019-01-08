#include <system/drivers/driver.h>

using namespace CactusOS;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

Driver::Driver(char* name, char* description)
{
    this->Name = name;
    this->Description = description;
}
                
char* Driver::GetDriverName() {
    return this->Name; 
}
char* Driver::GetDriverDescription() {
    return this->Description;
}

bool Driver::Initialize()
{
    //This should not be called
    return false;
}