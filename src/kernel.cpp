#include <system.h>
#include <multiboot/multiboot.h>
#include <common/convert.h>
#include <common/string.h>


using namespace CactusOS;
using namespace CactusOS::core;
using namespace CactusOS::common;
using namespace CactusOS::system;

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}

void printf(char* str)
{
    Console::Write(str);
}

void printfHex(uint8_t key)
{
    char* foo = "00";
    char* hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}
void printfHex16(uint16_t key)
{
    printfHex((key >> 8) & 0xFF);
    printfHex( key & 0xFF);
}
void printfHex32(uint32_t key)
{
    printfHex((key >> 24) & 0xFF);
    printfHex((key >> 16) & 0xFF);
    printfHex((key >> 8) & 0xFF);
    printfHex( key & 0xFF);
}

void PrintKernelStart()
{
    printf("Starting kernel at: ");
    printf(Convert::IntToString(RTC::GetHour())); printf(":"); printf(Convert::IntToString(RTC::GetMinute())); printf(":"); printf(Convert::IntToString(RTC::GetSecond()));
    printf("   ");
    printf(Convert::IntToString(RTC::GetDay())); printf(":"); printf(Convert::IntToString(RTC::GetMonth())); printf(":"); printf(Convert::IntToString(RTC::GetYear()));
    printf("\n");
}

void HandleUDPData(uint8_t* data, uint32_t size)
{
    printf("UDP Data: "); printf((char*)data); printf("\n");
}

extern "C" void kernelMain(const multiboot_info_t* mbi, unsigned int multiboot_magic)
{
    Console::SetColors(0xA, 0x0);

    PrintKernelStart();
    printf("CMD Parameters: "); printf((char*)mbi->cmdline); printf("\n");

    //Memory manager is needed for the new keyword
    uint32_t* memupper = (uint32_t*)(((uint32_t)mbi) + 8);
    uint32_t heap = 10*1024*1024;
    MemoryManager memoryManager(heap, (*memupper)*1024 - heap - 10*1024);
    printf("Memory Manager Loaded\n");

    printf("Starting Core\n");
    System::InitCore();

    printf("Starting System\n");
    System::InitSystem();

    Console::Write("Total Memory: "); Console::Write(Convert::IntToString(System::memoryManager->GetTotalMemory() / 1024 / 1024)); Console::WriteLine(" Mb");
    Console::Write("Free Memory: "); Console::Write(Convert::IntToString(System::memoryManager->GetFreeMemory() / 1024 / 1024)); Console::WriteLine(" Mb");
    Console::Write("Used Memory: "); Console::Write(Convert::IntToString(System::memoryManager->GetUsedMemory() > 1024 * 1024 ? System::memoryManager->GetUsedMemory() / 1024 / 1024 : System::memoryManager->GetUsedMemory() / 1024)); Console::WriteLine(System::memoryManager->GetUsedMemory() > 1024 * 1024 ? (char*)" Mb" : (char*)" Kb");
    
    while(1)
    {
        Console::Write(":==> ");
        char* input = Console::ReadLine();
        Console::Write("You typed: ");
        Console::WriteLine(input);
    }
}
