/*
https://wiki.osdev.org/Bochs_VBE_Extensions
*/ 

#include <system/components/bochsvbe.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::core;

BochsVBE::BochsVBE()
: GraphicsDevice("Bochs VBE Adapter"),
SystemComponent("BOCHS VBE", "Bochs VBE Extensions")
{ }

void WriteRegister(uint16_t IndexValue, uint16_t DataValue)
{
    outportw(VBE_DISPI_IOPORT_INDEX, IndexValue);
    outportw(VBE_DISPI_IOPORT_DATA, DataValue);
}
 
uint16_t ReadRegister(uint16_t IndexValue)
{
    outportw(VBE_DISPI_IOPORT_INDEX, IndexValue);
    return inportw(VBE_DISPI_IOPORT_DATA);
}
 
bool BochsVBE::IsAvailable()
{
    uint16_t id = ReadRegister(VBE_DISPI_INDEX_ID);
    return (id >= VBE_DISPI_ID0 && id <= VBE_DISPI_ID5);
}
 
void SetVideoMode(uint32_t Width, uint32_t Height, uint32_t BitDepth, int UseLinearFrameBuffer, int ClearVideoMemory)
{
    WriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    WriteRegister(VBE_DISPI_INDEX_XRES, Width);
    WriteRegister(VBE_DISPI_INDEX_YRES, Height);
    WriteRegister(VBE_DISPI_INDEX_BPP, BitDepth);
    WriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED |
        (UseLinearFrameBuffer ? VBE_DISPI_LFB_ENABLED : 0) |
        (ClearVideoMemory ? 0 : VBE_DISPI_NOCLEARMEM));
}
            
bool BochsVBE::SelectBestVideoMode()
{
    if(!IsAvailable())
        return false;
    
    SetVideoMode(VBE_DISPI_MAX_XRES, VBE_DISPI_MAX_YRES, 32, true, true);

    this->width = VBE_DISPI_MAX_XRES;
    this->height = VBE_DISPI_MAX_YRES;
    this->bpp = 32;
    this->framebufferPhys = VBE_DISPI_LFB_PHYSICAL_ADDRESS;

    return true;
}