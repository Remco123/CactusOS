#include <system/system.h>
#include <system/listings/directorylisting.h>
#include <system/listings/processlisting.h>
#include <system/listings/disklisting.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;
using namespace LIBCactusOS;

multiboot_info_t* System::mbi = 0;
PIT* System::pit = 0;
RTC* System::rtc = 0;
DMAController* System::dma = 0;
SMBIOS* System::smbios = 0;
Virtual8086Manager* System::vm86Manager = 0;
Virtual8086Monitor* System::vm86Monitor = 0;
GraphicsDevice* System::gfxDevice = 0;
EDID* System::edid = 0;
PCIController* System::pci = 0;
DriverManager* System::driverManager = 0;
DiskManager* System::diskManager = 0;
VFSManager* System::vfs = 0;
Scheduler* System::scheduler = 0;
APMController* System::apm = 0;
SystemCallHandler* System::syscalls = 0;
SharedSystemInfo* System::systemInfo = 0;

ScreenMode System::screenMode = ScreenMode::TextMode;
bool System::gdbEnabled = false;
bool System::setupMode = false;
Stream* System::keyboardStream = 0;
Stream* System::ProcStandardOut = 0;
List<ListingController*>* System::listings = 0;
USBManager* System::usbManager = 0;
#if BOCHS_GFX_HACK
bool System::isBochs = false; //are we running inside bochs
#endif

void System::Start()
{
    BootConsole::ForegroundColor = VGA_COLOR_BLACK;
    BootConsole::WriteLine("Adding system components");
    
    BootConsole::Write("RTC");
    System::rtc = new RTC();
    BootConsole::WriteLine(" [Done]");

    BootConsole::Write("PIT");
    InterruptDescriptorTable::DisableInterrupts();
    System::pit = new PIT();
    InterruptDescriptorTable::EnableInterrupts();
    BootConsole::WriteLine(" [Done]");

    BootConsole::WriteLine("DMA [Done]");
    System::dma = new DMAController();

    BootConsole::WriteLine("SMBIOS [Done]");
    System::smbios = new SMBIOS(true);
#if 0
//This does not work on bochs for some weird reason, we get a page-fault then.
    System::smbios->PrintSummary();
#endif

    BootConsole::WriteLine("Adding Virtual 8086");
    System::vm86Manager = new Virtual8086Manager();
    System::vm86Monitor = new Virtual8086Monitor();

    //The graphics component is added here but not used right away, we don't need to be in video mode so early.
    BootConsole::Write("Graphics Device");
    System::gfxDevice = GraphicsDevice::GetBestDevice();
    BootConsole::WriteLine(" [Done]");

    //Check for monitor EDID
    System::edid = new EDID();
    System::edid->AcquireEDID();

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

    BootConsole::WriteLine("Starting Scheduler");
    InterruptDescriptorTable::DisableInterrupts();
    System::scheduler = new Scheduler();
    InterruptDescriptorTable::EnableInterrupts();

    BootConsole::WriteLine("Starting USB Manager");
    System::usbManager = new USBManager();
    BootConsole::WriteLine("Initializing Virtual File System");
    System::vfs = new VFSManager();

    BootConsole::WriteLine("Setting up random...");
    Random::SetSeed(pit->Ticks());

    BootConsole::WriteLine("Assigning PCI Drivers");
    PCIDrivers::AssignDriversFromPCI(System::pci, System::driverManager);

    BootConsole::WriteLine("Creating shared region for system info");
    System::systemInfo = (SharedSystemInfo*)KernelHeap::allignedMalloc(PAGE_SIZE, PAGE_SIZE);
    MemoryOperations::memset(System::systemInfo, 0, PAGE_SIZE);

    BootConsole::WriteLine("Added drivers for integrated devices");
    System::driverManager->AddDriver(new MouseDriver());
    System::driverManager->AddDriver(new KeyboardDriver());
    System::driverManager->AddDriver(new FloppyDriver());
    
    BootConsole::WriteLine("Activating Drivers");
    System::driverManager->ActivateAll();

    BootConsole::WriteLine("Setting up found USB controllers");
    System::usbManager->SetupAll();
    BootConsole::WriteLine("Setting up found USB devices");
    System::usbManager->AssignAllDrivers();
    System::usbManager->USBPoll();

    //Advanced Power Management
    System::apm = new APMController();

    BootConsole::Write("Found a total of: "); BootConsole::Write(Convert::IntToString(System::diskManager->allDisks.size())); BootConsole::WriteLine(System::diskManager->allDisks.size() > 1 ? (char*)" disks" : (char*)" disk");
    BootConsole::Write("Searching for boot partition");
    if(System::vfs->SearchBootPartition()) {
        BootConsole::Write(" [Found] ("); BootConsole::Write(Convert::IntToString(System::vfs->bootPartitionID)); BootConsole::WriteLine(")");
    }
    else
        BootConsole::WriteLine(" [Not found]");

    BootConsole::WriteLine("Starting Systemcalls");
    System::syscalls = new SystemCallHandler();

    BootConsole::WriteLine("Preparing IPC");
    IPCManager::Initialize();

    BootConsole::WriteLine("Adding default listing handlers");
    System::listings = new List<ListingController*>();
    System::listings->push_back(new DirectoryListing());
    System::listings->push_back(new ProcessListing());
    System::listings->push_back(new DiskListing());

    System::ProcStandardOut = new StandardOutSteam();
    Log(Info, "System Initialized");
}