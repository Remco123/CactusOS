#include <system/initrd.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;

void* locationInMemory = 0;

void InitialRamDisk::Initialize(multiboot_info_t* mbi)
{
    if(mbi->mods_count <= 0)
    {
        BootConsole::WriteLine("Error mods count is 0 or less");
        return;
    }
    
    uint32_t ramdiskLocation = *(uint32_t*)mbi->mods_addr;
    uint32_t ramdiskEnd = *(uint32_t*)(mbi->mods_addr + 4);

    BootConsole::Write("Ramdisk is at: 0x"); Print::printfHex32(ramdiskLocation); BootConsole::WriteLine();
    BootConsole::Write("Ramdisk size: "); BootConsole::WriteLine(Convert::IntToString(ramdiskEnd - ramdiskLocation));

    locationInMemory = (void*)ramdiskLocation;
}

void* InitialRamDisk::ReadFile(const char* path, uint32_t* fileSizeReturn)
{
    InitrdFileHeader* header = (InitrdFileHeader*)locationInMemory;
    while(header->size != 0)
    {
        if(String::strcmp(header->path, path))
        {
            if(fileSizeReturn != 0)
                *fileSizeReturn = header->size;
            return (void*)((uint32_t)header + sizeof(InitrdFileHeader));
        }
        
        header = (InitrdFileHeader*)((uint32_t)header + sizeof(InitrdFileHeader) + header->size);
    }
    return 0;
}