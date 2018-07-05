#include <system/drivers/driver.h>

using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);

Driver::Driver()
{
    this->Type = DriverType::Unkown;
}
Driver::~Driver()
{
    
}

void Driver::Activate()
{
    printf("Warning! Driver::Activate() Called\n");
}