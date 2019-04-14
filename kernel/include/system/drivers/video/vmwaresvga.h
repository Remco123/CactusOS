#ifndef __CACTUSOS__SYSTEM__DRIVERS__VMWARESVGA_H
#define __CACTUSOS__SYSTEM__DRIVERS__VMWARESVGA_H

#include <system/drivers/driver.h>
#include <system/components/pci.h>
#include <system/interruptmanager.h>
#include <system/components/graphicsdevice.h>

namespace CactusOS
{
    namespace system
    {
        namespace drivers
        {
            #define VMWARESVGAII_VENDORID 0x15AD
            #define VMWARESVGAII_DEVICEID 0x0405

            #define SVGA_INDEX_PORT         0x0
            #define SVGA_VALUE_PORT         0x1
            #define SVGA_BIOS_PORT          0x2
            #define SVGA_IRQSTATUS_PORT     0x8

            #define SVGA_MAGIC         0x900000UL
            #define SVGA_MAKE_ID(ver)  (SVGA_MAGIC << 8 | (ver))

            /* Version 2 let the address of the frame buffer be unsigned on Win32 */
            #define SVGA_VERSION_2     2
            #define SVGA_ID_2          SVGA_MAKE_ID(SVGA_VERSION_2)

            #define SVGA_REG_ID 0
            #define SVGA_REG_FB_START 13
            #define SVGA_REG_FB_Enable 1
            #define SVGA_REG_FB_Width 2
            #define SVGA_REG_FB_Height 3
            #define SVGA_REG_FB_BitsPerPixel 7

            class VMWARESVGAII : public Driver, public GraphicsDevice
            {
            private:
                PCIDevice* pciDevice;

                void WriteRegister(common::uint32_t reg, common::uint32_t value);
                common::uint32_t ReadRegister(common::uint32_t reg);
            public:
                VMWARESVGAII(PCIDevice* pciDev);

                bool Initialize();
                bool SelectBestVideoMode();
            };
        }
    }
}

#endif