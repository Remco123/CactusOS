#include <system.h>
#include <multiboot/multiboot.h>
#include <common/convert.h>
#include <system/network/udp.h>
#include <system/network/dhcp.h>

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

void terminal_scroll(){
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;
    for(int i = 0; i < 25; i++){
        for (int m = 0; m < 80; m++){
            VideoMemory[i * 80 + m] = VideoMemory[(i + 1) * 80 + m];
        }
    }
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

void PrintIP(uint32_t ip)
{
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;   
    //printf("%d.%d.%d.%d\n", bytes[3], bytes[2], bytes[1], bytes[0]);
    printf(Convert::IntToString(bytes[3])); printf(".");
    printf(Convert::IntToString(bytes[2])); printf(".");
    printf(Convert::IntToString(bytes[1])); printf(".");
    printf(Convert::IntToString(bytes[0])); 
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

    while(1)
    {
        Console::Write(":==> ");
        char* input = Console::ReadLine();
        Console::Write("You typed: ");
        Console::WriteLine(input);
    }
}
