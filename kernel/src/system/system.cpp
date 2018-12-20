#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

PIT* System::pit = 0;
RTC* System::rtc = 0;

void System::Start()
{
    BootConsole::ForegroundColor = VGA_COLOR_GREEN;
    BootConsole::WriteLine("Adding system components");
    
    BootConsole::Write("    - RTC");
    System::rtc = new RTC();
    BootConsole::WriteLine(" [Done]");

    BootConsole::Write("    - PIT");
    System::pit = new PIT();
    BootConsole::WriteLine(" [Done]");
}