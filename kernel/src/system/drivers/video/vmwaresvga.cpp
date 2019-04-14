#include <system/drivers/video/vmwaresvga.h>

#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;
using namespace CactusOS::core;

VMWARESVGAII::VMWARESVGAII(PCIDevice* pciDev)
: Driver("VMWare SVGAII", "VMWare Graphics Addapter"),
  GraphicsDevice()
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
    BootConsole::WriteLine("Intializing VMWare Graphics Device");
    
    //Enable Device Memory
    uint16_t com = System::pci->Read(pciDevice->bus, pciDevice->device, pciDevice->function, 0x04);
    com |= 0x0007;
    System::pci->Write(pciDevice->bus, pciDevice->device, pciDevice->function, 0x04, com);

    WriteRegister(SVGA_REG_ID, SVGA_ID_2);
    if (ReadRegister(SVGA_REG_ID) != SVGA_ID_2)
        return false;

    this->framebufferPhys = ReadRegister(SVGA_REG_FB_START);
    BootConsole::Write("Framebuffer is at: 0x"); Print::printfHex32(this->framebufferPhys); BootConsole::WriteLine();

    BootConsole::WriteLine("Replacing fallback gfx device");
    GraphicsDevice* old = System::gfxDevice;
    System::gfxDevice = this;
    delete old;

    return true;
}

bool VMWARESVGAII::SelectBestVideoMode()
{
    WriteRegister(SVGA_REG_FB_Width, 1024);
    WriteRegister(SVGA_REG_FB_Height, 768);
    WriteRegister(SVGA_REG_FB_BitsPerPixel, 32);
    WriteRegister(SVGA_REG_FB_Enable, 1);

    this->width = 1024;
    this->height = 768;
    this->bpp = 32;
    
    return true;
}