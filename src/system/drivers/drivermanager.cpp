#include <system/drivers/drivermanager.h>

using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);
void printfHex16(uint16_t);

//When we add this in the header it doesn't work for some reason, I should really look into this!
uint32_t NumDrivers = 0;
Driver* drivers[32]; 

DriverManager::DriverManager()
{
    NumDrivers = 0;
}
void DriverManager::AddDriver(Driver* driver)
{
    drivers[NumDrivers] = driver;
    NumDrivers++;
}

void DriverManager::AssignDrivers(PeripheralComponentInterconnectController* pciController, InterruptManager* interrupts, PeripheralComponentInterconnectController* pci)
{
    if(pciController->NumDevices == 0)
        return;

    for(uint32_t i = 0; i < pciController->NumDevices; i++)
    {
        PeripheralComponentInterconnectDeviceDescriptor* dev = pciController->Devices[i];
        printf("Finding driver for device "); printf(Convert::IntToString(i));
        printf("    VendorID: "); printfHex16(dev->vendor_id); printf(" DeviceID: "); printfHex16(dev->device_id);

        Driver* driver = GetDriver(dev, interrupts, pci);
        if(driver != 0)
        {
            this->AddDriver(driver);
            printf("Driver added\n");
        }
        else
            printf("  [None found]\n");
    }

    printf("Loaded "); printf(Convert::IntToString(NumDrivers)); printf(" drivers\n");
}

Driver* DriverManager::GetDriver(core::PeripheralComponentInterconnectDeviceDescriptor* device, InterruptManager* interrupts, PeripheralComponentInterconnectController* pci)
{
    switch (device->vendor_id)
    {
        //case 0x1022: //Amd
        //    switch (device->device_id)
        //    {
        //        case 0x2000:
        //            printf("  [Loading...]\n");
        //            printf("Loading AMD_AM79C973 NIC\n");
        //            return new amd_am79c973(device, interrupts);
        //            break;
        //    }
        //    break;
        //#### This device is broken at the moment

        case 0x10EC: //Realtek
            switch(device->device_id)
            {
                case 0x8139: // rtl8139
                    printf("  [Loading...]\n");
                    printf("Loading RTL8139 NIC\n");
                    return new RTL8139(device, interrupts, pci);
                    break;
            }
            break;
    }
    return 0;
}

void DriverManager::ActivateAll()
{
    printf("Number of drivers to activate: "); printf(Convert::IntToString(NumDrivers)); printf("\n");
    for(int i = 0; i < NumDrivers; i++)
    {
        printf("Activating "); printf(Convert::IntToString(i)); printf("\n");
        drivers[i]->Activate(); //Every driver has this method
    }
}

Driver* DriverManager::DriverByType(DriverType type)
{
    printf("Finding driver with type: "); printf(Convert::IntToString(type)); printf("\n");
    for(uint32_t i = 0; i < NumDrivers; i++)
        if(drivers[i]->Type == type)
            return drivers[i];
            
    printf("Warning! Not found\n");
    return 0;
}