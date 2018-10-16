#include <system.h>
#include <multiboot/multiboot.h>
#include <common/list.h>

using namespace CactusOS;
using namespace CactusOS::core;
using namespace CactusOS::common;
using namespace CactusOS::system;

#define SerialPrint

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}

#ifdef SerialPrint
#define PORT 0x3f8   /* COM1 */
 
void init_serial() {
   outportb(PORT + 1, 0x00);    // Disable all interrupts
   outportb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
   outportb(PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
   outportb(PORT + 1, 0x00);    //                  (hi byte)
   outportb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
   outportb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
   outportb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

int is_transmit_empty() {
   return inportb(PORT + 5) & 0x20;
}
 
void write_serial(char a) {
   while (is_transmit_empty() == 0);
 
   outportb(PORT,a);
}
#endif

void printf(char* str)
{
    Console::Write(str);
    #ifdef SerialPrint
    for(int i = 0; str[i] != '\0'; ++i)
    {
        write_serial(str[i]);
    }
    #endif
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
void printbits(uint8_t key)
{
    for(int bit = 0;bit < 8; bit++)
    {
      printf(Convert::IntToString(key & 0x01));
      key = key >> 1;
    }
}


void PrintKernelStart()
{
    printf("Starting kernel at: ");
    printf(Convert::IntToString(RTC::GetHour())); printf(":"); printf(Convert::IntToString(RTC::GetMinute())); printf(":"); printf(Convert::IntToString(RTC::GetSecond()));
    printf("   ");
    printf(Convert::IntToString(RTC::GetDay())); printf(":"); printf(Convert::IntToString(RTC::GetMonth())); printf(":"); printf(Convert::IntToString(RTC::GetYear()));
    printf("\n");
}

extern "C" void kernelMain(const multiboot_info_t* mbi, unsigned int multiboot_magic)
{
    Console::SetColors(0xA, 0x0);

    PrintKernelStart();
    printf("CMD Parameters: "); printf((char*)mbi->cmdline); printf("\n");

    #ifdef SerialPrint
    init_serial();
    #endif

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
        
        if(String::strcmp(input, "ls"))
        {
            Console::Write("Path: ");
            List<char*>* dirs = System::vfsManager->DirectoryList(Console::ReadLine());
            if(dirs != 0)
            {
                for(int i = 0; i < dirs->size(); i++)
                {
                    printf(dirs->GetAt(i)); printf("\n");
                    delete dirs->GetAt(i);
                }
                delete dirs;
            }
        }
        else if(String::strcmp(input, "filesize"))
        {
            Console::Write("Filename: ");
            int length = System::vfsManager->GetFileSize(Console::ReadLine());
            printf("File size: "); printf(Convert::IntToString(length)); printf("\n");
        }
    }
}
