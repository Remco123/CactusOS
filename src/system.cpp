#include <system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);

sys_Core* System::core = 0;
sys_System* System::system = 0;

void System::InitCore()
{
    if(MemoryManager::activeMemoryManager == 0)
    {
        printf("Memory Manager is zero, error!\n");
        while(1); //TODO: Add panic
    }

    //We can use new now!
    System::core->gdt = new GlobalDescriptorTable();
    printf("GDT Loaded\n");

    System::core->cpu = new CPU();
    System::core->cpu->CollectInfo();
    printf("CPU Loaded\n");

    printf("Enabling CPU Specific features\n");
    System::core->cpu->EnableFeatures();

    printf("Finding SMBIOS\n");
    SMBIOS::Find();
    SMBIOS::BiosInfo();
    SMBIOS::CpuInfo();

    System::core->interrupts = new InterruptManager(0x20, System::core->gdt);
    printf("Interrupts Loaded\n");

    System::core->pit = new PIT(System::core->interrupts); //We need the interrupts controller for this
    printf("PIT Loaded\n");

    System::core->pci = new PeripheralComponentInterconnectController();
    System::core->pci->FindDevices();
    printf("PCI Loaded with "); printf(Convert::IntToString(System::core->pci->NumDevices)); printf(" Devices connected\n");
}

void System::InitSystem()
{
    System::system->driverManager = new DriverManager();
    printf("Driver manager loaded\n");
    System::system->driverManager->AssignDrivers(System::core->pci, System::core->interrupts, System::core->pci);
    System::system->driverManager->ActivateAll();

    //Activate interrupts after drivers are loaded
    System::core->interrupts->Activate();
    printf("Interrupts Activated\n");

    NetworkDriver* netDriver = (NetworkDriver*) System::system->driverManager->DriverByType(DriverType::Network);
    if(netDriver != 0)
    {
        System::system->networkManager = new NetworkManager(netDriver); //This way only 1 network device gets used
        printf("Network initialized\n");
    }
    else
        printf("No network device found so network is disabled\n");
}