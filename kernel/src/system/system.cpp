#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

PIT* System::pit = 0;
RTC* System::rtc = 0;
SMBIOS* System::smbios = 0;
Virtual8086Manager* System::vm86Manager = 0;
Virtual8086Monitor* System::vm86Monitor = 0;

void System::Start()
{
    BootConsole::ForegroundColor = VGA_COLOR_BLACK;
    BootConsole::WriteLine("Adding system components");
    
    BootConsole::Write("    - RTC");
    System::rtc = new RTC();
    BootConsole::WriteLine(" [Done]");

    BootConsole::Write("    - PIT");
    System::pit = new PIT();
    BootConsole::WriteLine(" [Done]");

    BootConsole::WriteLine("    - SMBIOS [Done]");
    System::smbios = new SMBIOS(true);
    System::smbios->PrintSummary();

    BootConsole::WriteLine("Adding Virtual 8086");
    System::vm86Manager = new Virtual8086Manager();
    System::vm86Monitor = new Virtual8086Monitor();

    BootConsole::WriteLine("Entering Graphics Mode");    
    VM86Registers regs;
    MemoryOperations::memset(&regs, 0, sizeof(VM86Registers));
    regs.AX = 0x4F02;
    regs.BX = 0x105;
    System::vm86Manager->CallInterrupt(0x10, &regs);
}