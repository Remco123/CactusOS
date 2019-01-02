#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

multiboot_info_t* System::mbi = 0;
PIT* System::pit = 0;
RTC* System::rtc = 0;
SMBIOS* System::smbios = 0;
Virtual8086Manager* System::vm86Manager = 0;
Virtual8086Monitor* System::vm86Monitor = 0;
VESA* System::vesa = 0;

void System::Start()
{
    BootConsole::ForegroundColor = VGA_COLOR_BLACK;
    BootConsole::WriteLine("Adding system components");
    
    BootConsole::Write("RTC");
    System::rtc = new RTC();
    BootConsole::WriteLine(" [Done]");

    BootConsole::Write("PIT");
    System::pit = new PIT();
    BootConsole::WriteLine(" [Done]");

    BootConsole::WriteLine("SMBIOS [Done]");
    System::smbios = new SMBIOS(true);
    System::smbios->PrintSummary();

    BootConsole::WriteLine("Adding Virtual 8086");
    System::vm86Manager = new Virtual8086Manager();
    System::vm86Monitor = new Virtual8086Monitor();

    //The vesa component is added here but not used right away, we don't need to be in video mode so early.
    BootConsole::Write("VESA VBE");
    System::vesa = new VESA(System::vm86Manager);
    BootConsole::WriteLine(" [Done]");

    BootConsole::WriteLine("Loading Initial Ramdisk");
    InitialRamDisk::Initialize(System::mbi);
}