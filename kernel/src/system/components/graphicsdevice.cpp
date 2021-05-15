#include <system/components/graphicsdevice.h>
#include <system/components/bochsvbe.h>
#include <system/components/vesa.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;

GraphicsDevice::GraphicsDevice(char* name)
{
    this->width = 0;
    this->height = 0;
    this->bpp = 0;
    this->identifier = name;
}

GraphicsDevice::~GraphicsDevice()
{
    
}

bool GraphicsDevice::SelectBestVideoMode()
{
    return false;
}

uint32_t GraphicsDevice::GetBufferSize()
{
	return this->width * this->height * (this->bpp/8);
}

//Select the best graphics device for the situation
GraphicsDevice* GraphicsDevice::GetBestDevice()
{
    #if BOCHS_GFX_HACK
    if(BochsVBE::IsAvailable() && System::isBochs)
    {
        //BootConsole::Write(" BochsVBE");
        return new BochsVBE();
    }
    #endif
    
    //BootConsole::Write(" VESA");
    return new VESA(System::vm86Manager);
}