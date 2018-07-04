#include <system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;

void printf(char*);

Core* System::core = 0;

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

    System::core->pit = new PIT(System::core->interrupts);
    printf("PIT Loaded\n");

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

    System::core->pci = new PeripheralComponentInterconnectController();
    System::core->pci->FindDevices();
    printf("PCI Loaded with "); printf(Convert::IntToString(System::core->pci->NumDevices)); printf(" Devices connected\n");
}