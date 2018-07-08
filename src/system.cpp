#include <system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);

//Static initialization
GlobalDescriptorTable* System::gdt = 0;
InterruptManager* System::interrupts = 0;
MemoryManager* System::memoryManager = 0;
PeripheralComponentInterconnectController* System::pci = 0;
PIT* System::pit = 0;
CPU* System::cpu = 0;
DriverManager* System::driverManager = 0;
NetworkManager* System::networkManager = 0;

void System::InitCore()
{
    if(MemoryManager::activeMemoryManager == 0)
    {
        printf("Memory Manager is zero, error!\n");
        while(1); //TODO: Add panic
    }

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
    System::driverManager->AssignDrivers(System::pci, System::interrupts, System::pci);
    System::driverManager->ActivateAll();

    //Activate interrupts after drivers are loaded
    System::interrupts->Activate();
    printf("Interrupts Activated\n");

    NetworkDriver* netDriver = (NetworkDriver*) System::driverManager->DriverByType(DriverType::Network);
    if(netDriver != 0)
    {
        uint32_t ip_be = ((uint32_t)15 << 24)
                | ((uint32_t)2 << 16)
                | ((uint32_t)0 << 8)
                | (uint32_t)10;
                
        System::networkManager = new NetworkManager(netDriver, ip_be); //This way only 1 network device gets used
        printf("Network initialized\n");
    }
    else
        printf("No network device found so network is disabled\n");
}