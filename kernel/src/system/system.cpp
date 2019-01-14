#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

multiboot_info_t* System::mbi = 0;
PIT* System::pit = 0;
RTC* System::rtc = 0;
SMBIOS* System::smbios = 0;
Virtual8086Manager* System::vm86Manager = 0;
Virtual8086Monitor* System::vm86Monitor = 0;
VESA* System::vesa = 0;
PCIController* System::pci = 0;
DriverManager* System::driverManager = 0;
DiskManager* System::diskManager = 0;
VFSManager* System::vfs = 0;

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
#if 0
//This does not work on bochs for some weird reason, we get a page-fault then.
    System::smbios->PrintSummary();
#endif

    BootConsole::WriteLine("Adding Virtual 8086");
    System::vm86Manager = new Virtual8086Manager();
    System::vm86Monitor = new Virtual8086Monitor();

    //The vesa component is added here but not used right away, we don't need to be in video mode so early.
    BootConsole::Write("VESA VBE");
    System::vesa = new VESA(System::vm86Manager);
    BootConsole::WriteLine(" [Done]");

    BootConsole::WriteLine("Loading Initial Ramdisk");
    InitialRamDisk::Initialize(System::mbi);

    BootConsole::Write("PCI");
    System::pci = new PCIController();
    BootConsole::WriteLine(" [Done]");

    System::pci->PopulateDeviceList();

    BootConsole::WriteLine("Starting Driver Manager");
    System::driverManager = new DriverManager();

    BootConsole::WriteLine("Starting Disk Manager");
    System::diskManager = new DiskManager();

    BootConsole::WriteLine("Assigning PCI Drivers");
    PCIDrivers::AssignDriversFromPCI(System::pci, System::driverManager);

    BootConsole::WriteLine("Activating Drivers");
    System::driverManager->ActivateAll();

    BootConsole::Write("Found a total of: "); BootConsole::Write(Convert::IntToString(System::diskManager->allDisks.size())); BootConsole::WriteLine(System::diskManager->allDisks.size() > 1 ? (char*)" disks" : (char*)" disk");
    BootConsole::WriteLine("Initializing Virtual File System");
    System::vfs = new VFSManager();

    PartitionManager::DetectAndLoadFilesystems(System::diskManager, System::vfs);
}