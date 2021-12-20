#ifndef __CACTUSOS__SYSTEM__SYSTEM_H
#define __CACTUSOS__SYSTEM__SYSTEM_H

#define BOCHS_GFX_HACK 0        // Enable or disable the bochs hack
#define ENABLE_USB 1            // Enable USB-Stack
#define ENABLE_MEMORY_CHECKS 1  // Enable the checking of memory on a specified interval
#define ENABLE_ADV_DEBUG 1      // Enable advanced debugging features

#include <system/bootconsole.h>
#include <system/components/systemcomponent.h>
#include <system/components/pit.h>
#include <system/components/rtc.h>
#include <system/components/smbios.h>
#include <system/components/graphicsdevice.h>
#include <system/components/vesa.h>
#include <system/components/edid.h>
#include <system/components/pci.h>
#include <system/components/apm.h>
#include <system/components/dma.h>
#include <system/virtual8086/VM86Manager.h>
#include <system/virtual8086/VM86Monitor.h>

#include <system/initrd.h>
#include <system/drivers/drivermanager.h>
#include <system/drivers/pcidrivers.h>
#include <system/drivers/integrated/ps2-mouse.h>
#include <system/drivers/integrated/ps2-keyboard.h>
#include <system/drivers/integrated/floppy.h>
#include <system/disks/diskmanager.h>
#include <system/disks/partitionmanager.h>
#include <system/vfs/vfsmanager.h>
#include <system/tasking/scheduler.h>
#include <system/syscalls/syscalls.h>
#include <common/random.h>
#include <system/tasking/ipcmanager.h>
#include <system/memory/sharedmem.h>
#include <system/listings/listingcontroller.h>
#include <system/usb/usbmanager.h>
#include <system/input/keyboardmanager.h>

#include <system/log.h>
#include <system/debugger.h>
#include <../../lib/include/systeminfo.h>

#define GDB_BREAK() asm("int $3");

namespace CactusOS
{
    namespace system
    {
        #define DEFAULT_SCREEN_WIDTH 1024
        #define DEFAULT_SCREEN_HEIGHT 768
        #define DEFAULT_SCREEN_BPP 32

        enum ScreenMode
        {
            TextMode,
            GraphicsMode
        };

        enum PowerRequest
        {
            None,
            Shutdown,
            Reboot
        };

        /**
         * The default stream where processes data is send to.
         * Data is send to the screen/serial output
        */
        class StandardOutSteam : public Stream
        {
        public:
            StandardOutSteam() : Stream() {}

            // We only overwrite the write function since reading is not supported
            void Write(char byte)
            {
                char str[1];
                str[0] = byte;
                Print(str, 1);
            }
        };     

        class System
        {
        public:
            static multiboot_info_t* mbi;
            static PIT* pit;
            static RTC* rtc;
            static SMBIOS* smbios;
            static DMAController* dma;
            static Virtual8086Manager* vm86Manager;
            static Virtual8086Monitor* vm86Monitor;
            static GraphicsDevice* gfxDevice;
            static EDID* edid;
            static PCIController* pci;
            static drivers::DriverManager* driverManager;
            static DiskManager* diskManager;
            static VFSManager* vfs;
            static Scheduler* scheduler;
            static SystemCallHandler* syscalls;
            static APMController* apm;
            static LIBCactusOS::SharedSystemInfo* systemInfo;

            static ScreenMode screenMode;
            static bool gdbEnabled; //Is the gdb stub enabled?
            static bool setupMode; //Are we running the setup program?
            static KeyboardManager* keyboardManager;
            static Stream* ProcStandardOut;
            static List<ListingController*>* listings;
            static USBManager* usbManager;
            static SymbolDebugger* kernelDebugger;
            #if BOCHS_GFX_HACK
            static bool isBochs; //are we running inside bochs
            #endif

            #if ENABLE_ADV_DEBUG
            typedef struct
            {
                // Values for measuring activity of idle process
                uint32_t idleProcCounter = 0;
                uint32_t idleProcStartTime = 0;
                uint32_t idleProcActive = 0;

                // Amount of disk read operations
                uint32_t diskReadOp = 0;

                // Amount of disk write operations
                uint32_t diskWriteOp = 0;
            } SYSTEM_STATS;
            static SYSTEM_STATS statistics;
            #endif

            static void Start();
            static void Panic();
        };
    }
} 

#endif