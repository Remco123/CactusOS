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

    printf("Adding keyboard to Console\n");
    Console::SetKeyboard((KeyboardDriver*)System::driverManager->DriverByType(DriverType::Keyboard));

    //Activate interrupts after drivers are loaded
    System::interrupts->Activate();
    Console::WriteLine("Interrupts Activated");

    
    NetworkDriver* netDriver = (NetworkDriver*) System::driverManager->DriverByType(DriverType::Network);
    if(netDriver != 0)
    {
        Console::WriteLine("Starting Network");

        //Default IP Address
        uint8_t ip1 = 10, ip2 = 0, ip3 = 2, ip4 = 15;
        //uint8_t ip1 = 192, ip2 = 168, ip3 = 2, ip4 = 32;
        uint32_t ip_be = ((uint32_t)ip4 << 24)
                    | ((uint32_t)ip3 << 16)
                    | ((uint32_t)ip2 << 8)
                    | (uint32_t)ip1;    

        // IP Address of the default gateway
        uint8_t gip1 = 10, gip2 = 0, gip3 = 2, gip4 = 2;
        //uint8_t gip1 = 192, gip2 = 168, gip3 = 2, gip4 = 254;
        uint32_t gip_be = ((uint32_t)gip4 << 24)
                   | ((uint32_t)gip3 << 16)
                   | ((uint32_t)gip2 << 8)
                   | (uint32_t)gip1;
    
        uint8_t subnet1 = 255, subnet2 = 255, subnet3 = 255, subnet4 = 0;
        uint32_t subnet_be = ((uint32_t)subnet4 << 24)
                   | ((uint32_t)subnet3 << 16)
                   | ((uint32_t)subnet2 << 8)
                    | (uint32_t)subnet1;
                
        System::networkManager = new NetworkManager(netDriver, System::pit, ip_be); //This way only 1 network device gets used
        System::networkManager->ipv4Handler = new IPV4Handler(System::networkManager, gip_be, subnet_be);

        System::networkManager->dhcpController = new DHCP(System::networkManager);

        /*
        Console::WriteLine("Enabling DHCP");
        for(int i = 0; i < DHCP_MAX_TRIES; i++)
        {
            if(!System::networkManager->dhcpController->Enabled)
            {
                System::networkManager->dhcpController->EnableDHCP();
                System::pit->Sleep(1000);
            }
        }
        if(System::networkManager->dhcpController->Enabled)
            Console::WriteLine("DHCP Enabled!");
        
        else
            { Console::Write("DHCP Timed out, using default ip: "); PrintIP(Convert::ByteSwap(ip_be)); Console::Write("\n"); }
        */
        Console::Write("Resolving: "); PrintIP(Convert::ByteSwap(gip_be)); Console::WriteLine();
        System::networkManager->arpHandler->BroadcastMACAddress(gip_be);
        System::networkManager->ipv4Handler->icmpHandler->RequestEchoReply(gip_be);
        Console::WriteLine("Network initialized");
    }
    else
        Console::WriteLine("No network device found so network is disabled");
    
}