#include <log.h>
#include <api.h>
#include <types.h>
#include <vfs.h>
#include <syscall.h>
#include <string.h>
#include <new.h>
#include <proc.h>
#include <ipc.h>
#include <time.h>
#include <math.h>

void PrintDiskInfo()
{
    Print("Disks found on system:\n");

    int diskCount = SystemInfo::Properties["disks"].size();
    for(int i = 0; i < diskCount; i++) {
        char* id = (char*)SystemInfo::Properties["disks"][i]["identifier"];
        uint64_t sz = (uint64_t)SystemInfo::Properties["disks"][i]["size"];

        Print("    %d: %s\n", i, id);
        
        if(sz < 1_MB)
            Print("        -> Size = %d Kb\n", sz / 1_KB);
        else if (sz < 1_GB)
            Print("        -> Size = %d Mb\n", sz / 1_MB);
        else
            Print("        -> Size = %d Gb\n", sz / 1_GB);

        Print("        -> Blocks = %d\n", (uint32_t)SystemInfo::Properties["disks"][i]["blocks"]);
        Print("        -> BlockSize = %d\n", (uint32_t)SystemInfo::Properties["disks"][i]["blocksize"]);

        switch((int)SystemInfo::Properties["disks"][i]["type"]) {
            case 0: {
                Print("        -> Type = HardDisk\n");
                break;
            }
            case 1: {
                Print("        -> Type = USB-MSD\n");
                break;
            }
            case 2: {
                Print("        -> Type = Floppy\n");
                break;
            }
            case 3: {
                Print("        -> Type = CDROM\n");
                break;
            }
        }

        delete id;
    }
}

void PrintUSBInfo()
{
    Print("USB Controllers found:\n");
    int ctrlCount = SystemInfo::Properties["usbcontrollers"].size();
    for(int i = 0; i < ctrlCount; i++) {
        switch((int)SystemInfo::Properties["usbcontrollers"][i]["type"]) {
            case 0: {
                Print("    %d: USB UHCI Controller\n", i);
                break;
            }
            case 1: {
                Print("    %d: USB OHCI Controller\n", i);
                break;
            }
            case 2: {
                Print("    %d: USB EHCI Controller\n", i);
                break;
            }
            case 3: {
                Print("    %d: USB xHCI Controller\n", i);
                break;
            }
        }
    }


    Print("USB Devices found:\n");
    int devCount = SystemInfo::Properties["usbdevices"].size();
    for(int i = 0; i < devCount; i++) {
        char* id = (char*)SystemInfo::Properties["usbdevices"][i]["name"];

        Print("    %d: %s\n", i, id);
        Print("        -> Address    = %d\n", (int)SystemInfo::Properties["usbdevices"][i]["address"]);
        Print("        -> Controller = %d\n", (int)SystemInfo::Properties["usbdevices"][i]["controller"]);
        Print("        -> VendorID   = %w\n", (uint16_t)SystemInfo::Properties["usbdevices"][i]["vendID"]);
        Print("        -> ProductID  = %w\n", (uint16_t)SystemInfo::Properties["usbdevices"][i]["prodID"]);
        Print("        -> Endpoints  = %d\n", (int)SystemInfo::Properties["usbdevices"][i]["vendID"].size());

        delete id;
    }
}

void PrintPCIInfo()
{
    Print("PCI Devices found:\n");
    int devCount = SystemInfo::Properties["pcidevs"].size();
    for(int i = 0; i < devCount; i++) {

        Print("    %d: %d:%d:%d   %w:%w   %b:%b    (Int = %b)\n", i, 
            (uint16_t)SystemInfo::Properties["pcidevs"][i]["bus"],
            (uint16_t)SystemInfo::Properties["pcidevs"][i]["device"],
            (uint16_t)SystemInfo::Properties["pcidevs"][i]["function"],
            (uint16_t)SystemInfo::Properties["pcidevs"][i]["vendorID"],
            (uint16_t)SystemInfo::Properties["pcidevs"][i]["deviceID"],
             (uint8_t)SystemInfo::Properties["pcidevs"][i]["classID"],
             (uint8_t)SystemInfo::Properties["pcidevs"][i]["subclassID"],
             (uint8_t)SystemInfo::Properties["pcidevs"][i]["interrupt"]);
    }
}

int main()
{
    Print("---------- Start of system information dump ----------\n");
    PrintDiskInfo();
    PrintUSBInfo();
    PrintPCIInfo();
    Print("------------------------------------------------------\n");
}