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

void PrintGFXInfo()
{
    Print("Current GFX Device:\n");

    char* name = (char*)SystemInfo::Properties["gfxdevice"]["name"];
    Print("  -> Name = %s\n", name);
    Print("  -> Bpp = %d\n", (uint8_t)SystemInfo::Properties["gfxdevice"]["bpp"]);
    Print("  -> Width = %d\n", (uint32_t)SystemInfo::Properties["gfxdevice"]["width"]);
    Print("  -> Height = %d\n", (uint32_t)SystemInfo::Properties["gfxdevice"]["height"]);
    Print("  -> Buffer = %x\n", (uint32_t)SystemInfo::Properties["gfxdevice"]["framebuffer"]);

    delete name;
}

void PrintPROCInfo()
{
    Print("Processes found:\n");
    int procCount = SystemInfo::Properties["processes"].size();
    for(int i = 0; i < procCount; i++) {
        char* id = (char*)SystemInfo::Properties["processes"][i]["filename"];

        Print("    %d: %s\n", i, id);
        Print("        -> Pid    = %d\n", (int)SystemInfo::Properties["processes"][i]["pid"]);
        Print("        -> Ring-3 = %d\n", (bool)SystemInfo::Properties["processes"][i]["userspace"]);
        Print("        -> State  = %d\n", (int)SystemInfo::Properties["processes"][i]["state"]);
        Print("        -> Base   = %x\n", (uint32_t)SystemInfo::Properties["processes"][i]["membase"]);
        Print("        -> Size   = %x\n", (uint32_t)SystemInfo::Properties["processes"][i]["memsize"]);
        Print("        -> Heap   = %d Kb\n", ((uint32_t)SystemInfo::Properties["processes"][i]["heap-end"] - (uint32_t)SystemInfo::Properties["processes"][i]["heap-start"]) / 1_KB);

        delete id;
    }
}

void PrintMEMInfo()
{
    Print("Memory Stats:\n");

    uint32_t total = (uint32_t)SystemInfo::Properties["memory"]["total"];
    uint32_t free = (uint32_t)SystemInfo::Properties["memory"]["free"];
    uint32_t used = (uint32_t)SystemInfo::Properties["memory"]["used"];

    Print("  -> Total = %d Mb\n", total / 1_MB);
    Print("  -> Free = %d MB\n", free / 1_MB);
    Print("  -> Used = %d Mb (%d%)\n", used / 1_MB, (uint32_t)(((double)used / (double)total) * 100.0));  
}

void PrintBIOSInfo()
{
    Print("BIOS Information:\n");

    char* vendor = (char*)SystemInfo::Properties["bios"]["vendor"];
    char* version = (char*)SystemInfo::Properties["bios"]["version"];
    char* release = (char*)SystemInfo::Properties["bios"]["releasedate"];

    Print("  -> Vendor = %s\n", vendor);
    Print("  -> Version = %s\n", version);
    Print("  -> Release = %s\n", release);

    delete vendor;
    delete version;
    delete release;
}

void PrintSystemInfo()
{
    Print("System Information:\n");

    char* manufacturer = (char*)SystemInfo::Properties["system"]["manufacturer"];
    char* product = (char*)SystemInfo::Properties["system"]["product"];
    char* version = (char*)SystemInfo::Properties["system"]["version"];
    char* serial = (char*)SystemInfo::Properties["system"]["serial"];
    char* sku = (char*)SystemInfo::Properties["system"]["sku"];
    char* family = (char*)SystemInfo::Properties["system"]["family"];

    Print("  -> Manufacturer = %s\n", manufacturer);
    Print("  -> Product = %s\n", product);
    Print("  -> Version = %s\n", version);
    Print("  -> Serial = %s\n", serial);
    Print("  -> Sku = %s\n", sku);
    Print("  -> Family = %s\n", family);
    
    delete manufacturer;
    delete product;
    delete version;
    delete serial;
    delete sku;
    delete family;
}

void PrintEnclosureInfo()
{
    Print("Enclosure Information:\n");

    char* manufacturer = (char*)SystemInfo::Properties["enclosure"]["manufacturer"];
    char* version = (char*)SystemInfo::Properties["enclosure"]["version"];
    char* serial = (char*)SystemInfo::Properties["enclosure"]["serial"];
    char* sku = (char*)SystemInfo::Properties["enclosure"]["sku"];

    Print("  -> Manufacturer = %s\n", manufacturer);
    Print("  -> Version = %s\n", version);
    Print("  -> Serial = %s\n", serial);
    Print("  -> Sku = %s\n", sku);
    
    delete manufacturer;
    delete version;
    delete serial;
    delete sku;
}

void PrintProcessorInfo()
{
    Print("Processor Information:\n");

    char* manufacturer = (char*)SystemInfo::Properties["processor"]["manufacturer"];
    char* socket = (char*)SystemInfo::Properties["processor"]["socket"];
    char* version = (char*)SystemInfo::Properties["processor"]["version"];

    Print("  -> Manufacturer = %s\n", manufacturer);
    Print("  -> Socket = %s\n", socket);
    Print("  -> Version = %s\n", version);
    
    delete manufacturer;
    delete socket;
    delete version;
}

int main(int argc, char** argv)
{
    Print("---------- Start of system information dump ----------\n");
    PrintDiskInfo();
    PrintUSBInfo();
    PrintPCIInfo();
    PrintGFXInfo();
    PrintPROCInfo();
    PrintMEMInfo();
    PrintBIOSInfo();
    PrintSystemInfo();
    PrintEnclosureInfo();
    PrintProcessorInfo();
    Print("------------------------------------------------------\n");
}