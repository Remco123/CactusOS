#ifndef __CACTUSOS__SYSTEM__COMPONENTS__VESA_H
#define __CACTUSOS__SYSTEM__COMPONENTS__VESA_H

#include <common/types.h>
#include <system/virtual8086/VM86Manager.h>
#include <system/components/graphicsdevice.h>
#include <core/virtualmemory.h>

namespace CactusOS
{
    namespace system
    {
        typedef struct
        {
            uint16_t A;
            uint16_t B;
        } __attribute__((packed)) Real_Pointer;

        struct VESAControllerInfo
        {
            char Signature[4];
            common::uint16_t Version;
            Real_Pointer OemStringPtr;
            common::uint32_t Capabilities;
            Real_Pointer VideoModePtr;
            common::uint16_t TotalMemory;
            common::uint16_t OemSoftwareRev;
            Real_Pointer OemVendorNamePtr;
            Real_Pointer OemProductNamePtr;
            Real_Pointer OemProductRevPtr;
        } __attribute__((packed));

        struct VESAModeInfo
        {
            common::uint16_t    ModeAttributes;
            common::uint8_t     WinAAttributes;
            common::uint8_t     WinBAttributes;
            common::uint16_t    WinGranularity;
            common::uint16_t    WinSize;
            common::uint16_t    WinASegment;
            common::uint16_t    WinBSegment;
            common::uint32_t    WinFuncPtr;
            common::uint16_t    BytesPerScanLine;
            common::uint16_t    XResolution;
            common::uint16_t    YResolution;
            common::uint8_t     XCharSize;
            common::uint8_t     YCharSize;
            common::uint8_t     NumberOfPlanes;
            common::uint8_t     BitsPerPixel;
            common::uint8_t     NumberOfBanks;
            common::uint8_t     MemoryModel;
            common::uint8_t     BankSize;
            common::uint8_t     NumberOfImagePages;
            common::uint8_t     Reserved_page;
            common::uint8_t     RedMaskSize;
            common::uint8_t     RedMaskPos;
            common::uint8_t     GreenMaskSize;
            common::uint8_t     GreenMaskPos;
            common::uint8_t     BlueMaskSize;
            common::uint8_t     BlueMaskPos;
            common::uint8_t     ReservedMaskSize;
            common::uint8_t     ReservedMaskPos;
            common::uint8_t     DirectColorModeInfo;
            common::uint32_t    PhysBasePtr;
            common::uint32_t    OffScreenMemOffset;
            common::uint16_t    OffScreenMemSize;
            common::uint8_t     Reserved[206];
        } __attribute__ ((packed));

        class VESA : public GraphicsDevice, public SystemComponent
        {
        private:
            Virtual8086Manager* virtual8086Manager;
        public:
            VESAModeInfo currentVideoMode;
            VESA(Virtual8086Manager* vm86);

            VESAModeInfo* GetModeInfo(common::uint16_t mode);
            void SetVideoMode(common::uint16_t mode);
            bool SelectBestVideoMode();
        };
    }
}

#endif