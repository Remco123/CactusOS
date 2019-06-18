#ifndef __CACTUSOS__SYSTEM__SYSTEM_H
#define __CACTUSOS__SYSTEM__SYSTEM_H

#define BOCHS_GFX_HACK 0 //Enable or disable the bochs hack

#include <system/bootconsole.h>
#include <system/components/systemcomponent.h>
#include <system/components/pit.h>
#include <system/components/rtc.h>
#include <system/components/smbios.h>
#include <system/components/graphicsdevice.h>
#include <system/components/vesa.h>
#include <system/components/pci.h>
#include <system/virtual8086/VM86Manager.h>
#include <system/virtual8086/VM86Monitor.h>

#include <system/initrd.h>
#include <system/drivers/drivermanager.h>
#include <system/drivers/pcidrivers.h>
#include <system/drivers/integrated/mouse.h>
#include <system/drivers/integrated/keyboard.h>
#include <system/disks/diskmanager.h>
#include <system/disks/partitionmanager.h>
#include <system/vfs/vfsmanager.h>
#include <system/tasking/scheduler.h>
#include <system/syscalls/syscalls.h>
#include <common/random.h>
#include <system/memory/sharedmem.h>

#include <system/log.h>
#include <../../lib/include/systeminfo.h>

#define GDB_BREAK() asm("int $3");

namespace CactusOS
{
    namespace system
    {
        enum ScreenMode
        {
            TextMode,
            GraphicsMode
        };        

        class System
        {
        public:
            static multiboot_info_t* mbi;
            static PIT* pit;
            static RTC* rtc;
            static SMBIOS* smbios;
            static Virtual8086Manager* vm86Manager;
            static Virtual8086Monitor* vm86Monitor;
            static GraphicsDevice* gfxDevice;
            static PCIController* pci;
            static drivers::DriverManager* driverManager;
            static DiskManager* diskManager;
            static VFSManager* vfs;
            static Scheduler* scheduler;
            static SystemCallHandler* syscalls;
            static LIBCactusOS::SharedSystemInfo* systemInfo;

            static ScreenMode screenMode;
            static bool gdbEnabled; //Is the gdb stub enabled?
            #if BOCHS_GFX_HACK
            static bool isBochs; //are we running inside bochs
            #endif

            static void Start();
        };
    }
} 

#endif