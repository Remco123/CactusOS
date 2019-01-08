#include <system/drivers/drivermanager.h>

using namespace CactusOS;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

DriverManager::DriverManager()
{
    this->driverList.Clear();
}

void DriverManager::AddDriver(Driver* drv)
{
    this->driverList.push_back(drv);
}

void DriverManager::ActivateAll()
{
    for(int i = 0; i < driverList.size(); i++)
    {
        if(driverList[i]->Initialize() == false)
        {
            uint8_t oldColor = BootConsole::ForegroundColor;
            BootConsole::ForegroundColor = VGA_COLOR_BLUE;
            BootConsole::Write("Warning: driver initialize failed for driver: "); BootConsole::WriteLine(driverList[i]->GetDriverName());
            BootConsole::ForegroundColor = oldColor;
        }
    }
}