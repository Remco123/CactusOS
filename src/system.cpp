#include <system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void PrintIP(uint32_t);

//Variables
bool System::NetworkAvailible = false;

//Static initialization
GlobalDescriptorTable* System::gdt = 0;
InterruptManager* System::interrupts = 0;
MemoryManager* System::memoryManager = 0;
PeripheralComponentInterconnectController* System::pci = 0;
PIT* System::pit = 0;
CPU* System::cpu = 0;
DriverManager* System::driverManager = 0;
NetworkManager* System::networkManager = 0;
DiskManager* System::diskManager = 0;

void System::InitCore()
{
    if(MemoryManager::activeMemoryManager == 0)
    {
        printf("Memory Manager is zero, error!\n");
        while(1); //TODO: Add panic
    }
    System::memoryManager = MemoryManager::activeMemoryManager;

    //We can use new now!
    System::gdt = new GlobalDescriptorTable();
    printf("GDT Loaded\n");

    System::cpu = new CPU();
    System::cpu->CollectInfo();
    printf("CPU Loaded\n");

    printf("Enabling CPU Specific features\n");
    System::cpu->EnableFeatures();

    printf("Finding SMBIOS\n");
    SMBIOS::Find();
    SMBIOS::BiosInfo();
    SMBIOS::CpuInfo();

    System::interrupts = new InterruptManager(0x20, System::gdt);
    printf("Interrupts Loaded\n");

    System::pit = new PIT(System::interrupts); //We need the interrupts controller for this
    printf("PIT Loaded\n");

    System::pci = new PeripheralComponentInterconnectController();
    System::pci->FindDevices();
    printf("PCI Loaded with "); printf(Convert::IntToString(System::pci->NumDevices)); printf(" Devices connected\n");
}

void System::InitSystem()
{
    System::driverManager = new DriverManager();
    printf("Driver manager loaded\n");
    System::driverManager->AssignDrivers(System::pci, System::interrupts);
    System::driverManager->ActivateAll();

    printf("Adding keyboard to Console\n");
    Console::SetKeyboard((KeyboardDriver*)System::driverManager->DriverByType(DriverType::Keyboard));

    //Activate interrupts after drivers are loaded
    System::interrupts->Activate();
    Console::WriteLine("Interrupts Activated");

    NetworkDriver* netDriver = (NetworkDriver*) System::driverManager->DriverByType(DriverType::Network);
    if(netDriver != 0)
    {
        Console::WriteLine("Starting Network");
        System::networkManager = new NetworkManager(netDriver);
        System::NetworkAvailible = System::networkManager->StartNetwork(System::pit);
    }
    else
    {
        Console::WriteLine("No network device found so network is disabled");
        System::NetworkAvailible = false;
    }

    Console::WriteLine("Loading diskmanager");
    System::diskManager = new DiskManager();
    System::diskManager->DetectAndLoadDisks(System::interrupts, System::pit);
}