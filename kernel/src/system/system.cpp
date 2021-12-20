#include <system/system.h>
#include <system/listings/directorylisting.h>

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
KeyboardManager* System::keyboardManager = 0;
Stream* System::ProcStandardOut = 0;
List<ListingController*>* System::listings = 0;
USBManager* System::usbManager = 0;
SymbolDebugger* System::kernelDebugger = 0;
#if BOCHS_GFX_HACK
bool System::isBochs = false; //are we running inside bochs
#endif
System::SYSTEM_STATS System::statistics = {};

void System::Start()
{
    BootConsole::ForegroundColor = VGA_COLOR_BLACK;
    Log(Info, "Adding system components");
    
    System::rtc = new RTC();
    Log(Info, "- RTC [Done]     (%x)", (uint32_t)System::rtc);

    InterruptDescriptorTable::DisableInterrupts();
    System::pit = new PIT();
    InterruptDescriptorTable::EnableInterrupts();
    Log(Info, "- PIT [Done]     (%x)", (uint32_t)System::pit);

    System::dma = new DMAController();
    Log(Info, "- DMA [Done]     (%x)", (uint32_t)System::dma);

    System::smbios = new SMBIOS();
    Log(Info, "- SMBIOS [Done]     (%x)", (uint32_t)System::smbios);

    Log(Info, "Adding Virtual 8086");
    System::vm86Manager = new Virtual8086Manager();
    System::vm86Monitor = new Virtual8086Monitor();

    // The graphics component is added here but not used right away, we don't need to be in video mode so early.
    System::gfxDevice = GraphicsDevice::GetBestDevice();
    Log(Info, "- GFX [Done]     (%x)", (uint32_t)System::gfxDevice);

    // Check for monitor EDID
    System::edid = new EDID();
    Log(Info, "- EDID [Done]     (%x)", (uint32_t)System::edid);
    System::edid->AcquireEDID();

    Log(Info, "Loading Initial Ramdisk");
    InitialRamDisk::Initialize(System::mbi);

    System::pci = new PCIController();
    Log(Info, "- PCI [Done]     (%x)", (uint32_t)System::pci);

    System::pci->PopulateDeviceList();

    Log(Info, "Starting Driver Manager");
    System::driverManager = new DriverManager();

    Log(Info, "Starting Disk Manager");
    System::diskManager = new DiskManager();

    Log(Info, "Starting Keyboard Manager");
    System::keyboardManager = new KeyboardManager();

    Log(Info, "Starting Scheduler");
    InterruptDescriptorTable::DisableInterrupts();
    System::scheduler = new Scheduler();
    InterruptDescriptorTable::EnableInterrupts();

    Log(Info, "Starting USB Manager");
    System::usbManager = new USBManager();
    Log(Info, "Initializing Virtual File System");
    System::vfs = new VFSManager();

    Log(Info, "Setting up random...");
    Random::SetSeed(pit->Ticks());

    Log(Info, "Assigning PCI Drivers");
    PCIDrivers::AssignDriversFromPCI(System::pci, System::driverManager);

    Log(Info, "Creating shared region for system info");
    System::systemInfo = (SharedSystemInfo*)KernelHeap::alignedMalloc(PAGE_SIZE, PAGE_SIZE);
    MemoryOperations::memset(System::systemInfo, 0, PAGE_SIZE);

    Log(Info, "Added drivers for integrated devices");
    System::driverManager->AddDriver(new PS2MouseDriver());
    System::driverManager->AddDriver(new PS2KeyboardDriver());
    System::driverManager->AddDriver(new FloppyDriver());
    
    Log(Info, "Activating Drivers");
    System::driverManager->ActivateAll();

    Log(Info, "Setting up found USB controllers");
    System::usbManager->SetupAll();
    Log(Info, "Setting up found USB devices");
    System::usbManager->AssignAllDrivers();
    System::usbManager->USBPoll();

    // Advanced Power Management
    System::apm = new APMController();

    Log(Info, "Found a total of: %d disks", System::diskManager->allDisks.size());
    BootConsole::Write("Searching for boot partition");
    if(System::vfs->SearchBootPartition()) {
        BootConsole::Write(" [Found] ("); BootConsole::Write(Convert::IntToString(System::vfs->bootPartitionID)); BootConsole::WriteLine(")");
    }
    else
        BootConsole::WriteLine(" [Not found]");

    Log(Info, "Starting Debugger");
    System::kernelDebugger = new SymbolDebugger("B:\\debug.sym", true);

    Log(Info, "Starting Systemcalls");
    System::syscalls = new SystemCallHandler();

    Log(Info, "Preparing IPC");
    IPCManager::Initialize();

    Log(Info, "Adding default listing handlers");
    System::listings = new List<ListingController*>();
    System::listings->push_back(new DirectoryListing());

    System::ProcStandardOut = new StandardOutSteam();
    Log(Info, "System Initialized");
}
void System::Panic()
{
    Log(Error, "-------------------------------");
    Log(Error, "--------- Kernel Halted -------");
    Log(Error, "-------------------------------");

    InterruptDescriptorTable::DisableInterrupts();
    while(1) {
        asm("hlt");
    }
}