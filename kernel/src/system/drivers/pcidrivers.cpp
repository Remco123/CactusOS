#include <system/drivers/pcidrivers.h>

//Drivers
#include <system/drivers/disk/ide.h>
#include <system/drivers/video/vmwaresvga.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;


PCIAttachedDriverEntry pciDriverList[] = 
{
    {VMWARESVGAII_VENDORID, VMWARESVGAII_DEVICEID, "VMWare SVGAII"}
};

const int pciDriverListCount = sizeof(pciDriverList) / sizeof(PCIAttachedDriverEntry);

void PCIDrivers::AssignDriversFromPCI(PCIController* pci, DriverManager* driverManager)
{
    for(int i = 0; i < pci->deviceList.size(); i++)
    {
        PCIDevice* pciDevice = pci->deviceList[i];

        //First loop through the known drivers per pci device
        for(int x = 0; x < pciDriverListCount; x++)
        {
            if(pciDriverList[x].vendorID == pciDevice->vendorID && pciDriverList[x].deviceID == pciDevice->deviceID && pciDriverList[x].driverString != 0)
            {
                //Found a driver for that specific device
                if(String::strcmp(pciDriverList[x].driverString, "VMWare SVGAII"))
                    driverManager->AddDriver(new VMWARESVGAII(pciDevice));

                goto FoundDriver;
            }
        }

        //Then check if we can assign a driver by its class and subclass
        switch(pciDevice->classID)
        {
            case 0x01: //Mass Storage Controller
            {
                switch(pciDevice->subclassID)
                {
                    case 0x01: //IDE Controller
                    {
                        driverManager->AddDriver(new IDEController(pciDevice));
                        goto FoundDriver;
                        break;
                    }
                }
                break;
            }
        }

FoundDriver:
        //Goto the next device in the list
        continue;
    }
}