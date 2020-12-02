#include <system/drivers/video/vmwaresvga.h>

#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;
using namespace CactusOS::core;

VMWARESVGAII::VMWARESVGAII(PCIDevice* pciDev)
: Driver("VMWare SVGAII", "VMWare Graphics Adapter"),
  GraphicsDevice("WMWare SVGAII Adapter")
{
    this->width = 0;
    this->height = 0;
    this->bpp = 0;
    this->framebufferPhys = 0;

    this->pciDevice = pciDev;
}

void VMWARESVGAII::WriteRegister(uint32_t index, uint32_t value)
{
    outportl(pciDevice->portBase + SVGA_INDEX_PORT, index);
    outportl(pciDevice->portBase + SVGA_VALUE_PORT, value);
}
uint32_t VMWARESVGAII::ReadRegister(uint32_t index)
{
    outportl(pciDevice->portBase + SVGA_INDEX_PORT, index);
    return inportl(pciDevice->portBase + SVGA_VALUE_PORT);
}   

bool VMWARESVGAII::Initialize()
{
    Log(Info, "Intializing VMWare Graphics Device");
    
    //Enable Device Memory
    uint16_t com = System::pci->Read(pciDevice->bus, pciDevice->device, pciDevice->function, 0x04);
    com |= 0x0007;
    System::pci->Write(pciDevice->bus, pciDevice->device, pciDevice->function, 0x04, com);

    WriteRegister(SVGA_REG_ID, SVGA_ID_2);
    if (ReadRegister(SVGA_REG_ID) != SVGA_ID_2)
        return false;

    this->framebufferPhys = ReadRegister(SVGA_REG_FB_START);
    Log(Info, "Framebuffer is at: %x", this->framebufferPhys);

    Log(Warning, "Replacing fallback gfx device");
    GraphicsDevice* old = System::gfxDevice;
    System::gfxDevice = this;
    delete old;

    return true;
}

bool VMWARESVGAII::SelectBestVideoMode()
{
    WriteRegister(SVGA_REG_FB_Width, DEFAULT_SCREEN_WIDTH);
    WriteRegister(SVGA_REG_FB_Height, DEFAULT_SCREEN_HEIGHT);
    WriteRegister(SVGA_REG_FB_BitsPerPixel, DEFAULT_SCREEN_BPP);
    WriteRegister(SVGA_REG_FB_Enable, 1);

    this->width = DEFAULT_SCREEN_WIDTH;
    this->height = DEFAULT_SCREEN_HEIGHT;
    this->bpp = DEFAULT_SCREEN_BPP;
    
    return true;
}