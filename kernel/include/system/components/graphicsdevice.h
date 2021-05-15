#ifndef __CACTUSOS__SYSTEM__COMPONENTS__GRAPHICSDEVICE_H
#define __CACTUSOS__SYSTEM__COMPONENTS__GRAPHICSDEVICE_H

#include <system/components/systemcomponent.h>

namespace CactusOS
{
    namespace system
    {
        class GraphicsDevice
        {
        public:
            common::uint32_t width;
            common::uint32_t height;
            common::uint8_t bpp;
            common::uint32_t framebufferPhys;
            char* identifier = 0;

            GraphicsDevice(char* name);
            virtual ~GraphicsDevice();
            virtual bool SelectBestVideoMode();
            
            common::uint32_t GetBufferSize();
            
            static GraphicsDevice* GetBestDevice();
        };
    }
}

#endif