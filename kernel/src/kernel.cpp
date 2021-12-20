#include <multiboot/multiboot.h>
#include <core/gdt.h>
#include <core/tss.h>
#include <core/idt.h>
#include <core/physicalmemory.h>
#include <core/virtualmemory.h>
#include <system/bootconsole.h>
#include <system/memory/heap.h>
#include <system/memory/new.h>
#include <system/system.h>
#include <common/list.h>
#include <common/print.h>
#include <common/convert.h>
#include <core/cpu.h>
#include <core/fpu.h>
#include <core/power.h>
#include <installer/installer.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

extern "C" uint32_t _kernel_base;
extern "C" uint32_t _kernel_end;
extern "C" uint32_t _kernel_virtual_base;
extern "C" uint32_t stack_top;

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}

extern "C" void _set_debug_traps();

PowerRequest powerRequestState;
void IdleThread()
{
    powerRequestState = None;
    uint64_t prevTicks = System::pit->Ticks();
    while(1) {
        uint64_t ticks = System::pit->Ticks();
        if(System::usbManager)
            System::usbManager->USBPoll();
                
        if(ticks - prevTicks > 500) {
            if(System::apm->Enabled)
                System::apm->CheckAndHandleEvents();

            #if ENABLE_MEMORY_CHECKS
            if(KernelHeap::CheckForErrors() == true) {
                Log(Error, "Memory is not intact anymore, halting system!");
                System::Panic();
            }
            #endif

            prevTicks = ticks;
        }

        // Handle power state requests from userspace
        // Processes can not do this themself because the first mb of memory is not mapped for them
        // And that should not be the case due to security issues :)
        if(powerRequestState == Reboot) {
            Power::Reboot();
        }
        if(powerRequestState == Shutdown) {
            Power::Poweroff();
        }
#if ENABLE_ADV_DEBUG
        // If we are sending messages over the serial port that don't go to gdb
        // We can send debug statistics to the debugging system
        if(Serialport::Initialized && !System::gdbEnabled) {
            System::kernelDebugger->Update();
        }

        // Calculate system utilisation by measuring how much the idle process is active
        System::statistics.idleProcCounter += 1;
        if (ticks - System::statistics.idleProcStartTime > 1000) {
            System::statistics.idleProcActive = System::statistics.idleProcCounter;
            System::statistics.idleProcCounter = 0;
            System::statistics.idleProcStartTime = ticks;
        }
#endif
        // Move onto other threads since there is nothing else to do here
        System::scheduler->ForceSwitch();
    }
}

extern "C" void kernelMain(const multiboot_info_t* mbi, unsigned int multiboot_magic)
{
    // Basic kernel information gathered from the linker script
    uint32_t kernel_base = (uint32_t) &_kernel_base;
    uint32_t kernel_end = (uint32_t) &_kernel_end;
    uint32_t kernel_size = kernel_end - kernel_base;

    //////////////
    // Check Kernel Arguments
    //////////////
    const char* args = (const char*)phys2virt(mbi->cmdline);

    if(String::strncmp(args, "gdb", 4)) {
        System::gdbEnabled = true;
        
        Serialport::Init(COM1); // Init serial port
        BootConsole::Init(false); // But don't use it for debug messages
    }
    else if(String::strncmp(args, "serial", 7))
        BootConsole::Init(true);

    BootConsole::ForegroundColor = VGA_COLOR_BLUE;
    BootConsole::BackgroundColor = VGA_COLOR_LIGHT_GREY;
    BootConsole::Clear();

    if(multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC)
    {
        BootConsole::WriteLine("Error: not booted by a multiboot bootloader");
        return;
    }

    BootConsole::WriteLine("Starting Kernel");
    BootConsole::Write("Built on: "); BootConsole::WriteLine(__DATE__ "  " __TIME__);

    BootConsole::Write("Kernel virtual base: 0x"); Print::printfHex32(_kernel_virtual_base); BootConsole::WriteLine();
    BootConsole::Write("Kernel Base: 0x"); Print::printfHex32(kernel_base); BootConsole::WriteLine();
    BootConsole::Write("Kernel End: 0x"); Print::printfHex32(kernel_end); BootConsole::WriteLine();
    BootConsole::Write("Kernel Size: "); BootConsole::Write(Convert::IntToString(kernel_size / 1_KB)); BootConsole::Write(" Kb      ("); BootConsole::Write(Convert::IntToString(kernel_size)); BootConsole::WriteLine(")");

    BootConsole::Write("GRUB Command Line Arguments: ");
    BootConsole::WriteLine((char*)phys2virt(mbi->cmdline));

    BootConsole::Write("Boot Device: "); Print::printfHex((mbi->boot_device & 0xFF000000) >> 24);
    BootConsole::WriteLine();

    GlobalDescriptorTable::Init();
    BootConsole::WriteLine("GDT Loaded");

    TSS::Install(5, 0x10, (uint32_t)&stack_top);
    BootConsole::WriteLine("TSS Loaded");

    InterruptDescriptorTable::Install();
    BootConsole::WriteLine("IDT Loaded");

    if (System::gdbEnabled) {
        BootConsole::WriteLine("Starting GDB Interrupts");
        _set_debug_traps();

        InterruptDescriptorTable::EnableInterrupts();
        
        BootConsole::WriteLine("Waiting for GDB connection");
        GDB_BREAK();
    }

    PhysicalMemoryManager::Initialize(mbi->mem_upper * 1_KB, kernel_end);
    BootConsole::WriteLine("Physical Memory Loaded");

    BootConsole::WriteLine("Reading CPU Info");
    CPU::PrintVendor();
    CPU::EnableFeatures();

    BootConsole::WriteLine("Enabling FPU");
    FPU::Enable();

    // Parse the memory map handled by grub
    PhysicalMemoryManager::ParseMemoryMap(mbi);

    // Protect the first 1mb of physical memory + the end of kernel. 
    // We also add the size of the bitmap so that it does not have to be staticly allocated, this makes the kernel way smaller. 
    // Also round it to page bounds.
    PhysicalMemoryManager::SetRegionUsed(0x0, pageRoundUp(1_MB + kernel_size + PhysicalMemoryManager::GetBitmapSize()));
    PhysicalMemoryManager::SetRegionUsed(*(uint32_t*)phys2virt(mbi->mods_addr), *(uint32_t*)phys2virt(mbi->mods_addr + 4) - *(uint32_t*)phys2virt(mbi->mods_addr));

    InterruptDescriptorTable::EnableInterrupts();
    BootConsole::WriteLine("Interrupts Enabled");

    VirtualMemoryManager::Initialize();
    BootConsole::WriteLine("Virtual Memory Loaded");

    KernelHeap::Initialize(KERNEL_HEAP_START, KERNEL_HEAP_START + KERNEL_HEAP_SIZE);
    Log(Info, "Kernel Heap Initialized");

    // From here we (should) only use the Log function for logging
    Log(Info, "Switching to log function based output");

    Power::Initialize();
    Log(Info, "Power Control Loaded");

    Log(Info, "Passing mbi to system");
    System::mbi = (multiboot_info_t*)KernelHeap::malloc(sizeof(multiboot_info_t));
    MemoryOperations::memcpy(System::mbi, mbi, sizeof(multiboot_info_t));

    BootConsole::ForegroundColor = VGA_COLOR_MAGENTA;
    Log(Info, "-Kernel core intialized-");

    // Further intialisation is done in the system class
    System::Start();

    Log(Info, "Loading Kernel Process");
    Process* kernelProcess = ProcessHelper::CreateKernelProcess();
    kernelProcess->Threads.push_back(ThreadHelper::CreateFromFunction(IdleThread, true));
    kernelProcess->Threads[0]->parent = kernelProcess;
    System::scheduler->AddThread(kernelProcess->Threads[0], false);

    // Check if we have found the directory with all the required stuff
    if(System::vfs->bootPartitionID == -1) {
        Log(Error, "Boot partition not found/present");
        
        // Just start running the idle thread so we at least detect device changes
        System::scheduler->ForceSwitch();
    }

    // Check if kernel is run from HardDisk
    // If not than ask the user if they would like to run the installer
    // Otherwise we run the liveCD
    if(System::vfs->Filesystems->GetAt(System::vfs->bootPartitionID)->disk->type != HardDisk) {
        // Prompt user
        BootConsole::ForegroundColor = VGA_COLOR_BLUE;
        System::setupMode = true;
        BootConsole::WriteLine("Press Enter to run Installer\nStarting LiveCD in 5 seconds....");

        int timeout = 0;
        while(System::keyboardManager->Available() == 0 && timeout < 5000) {
            System::pit->Sleep(100);
            timeout += 100;
            BootConsole::Write("#");
        }
        BootConsole::WriteLine();

        if(System::keyboardManager->Available() > 0) { // User pressed key
            uint8_t keyCode = System::keyboardManager->Read();    
            
            if(keyCode == KEY_ENTER) { // Return key
                BootConsole::WriteLine("Running Installer...");
                Installer::Run();
            }
        }
        BootConsole::ForegroundColor = VGA_COLOR_BLACK;
        BootConsole::WriteLine("Running LiveCD....");
        System::setupMode = false;
    }

    Log(Info, "Loading Init.bin");
    Process* proc = ProcessHelper::Create("B:\\apps\\init.bin");
    if(proc != 0)
    {
        if(System::gfxDevice->SelectBestVideoMode() == false) {
            Log(Error, "Could not set a video mode, halting system");
            System::Panic();
        }

        Log(Info, "Switched to graphics mode, phys=%x", System::gfxDevice->framebufferPhys);
        System::screenMode = ScreenMode::GraphicsMode;

        System::systemInfo->MouseX = System::gfxDevice->width/2;
        System::systemInfo->MouseY = System::gfxDevice->height/2;

        System::scheduler->AddThread(proc->Threads[0], true);
    }
    
    Log(Error, "Could not load process init.bin, halting system");
    System::Panic();
}