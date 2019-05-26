#ifndef __LIBCACTUSOS__GUI__GUICOM_H
#define __LIBCACTUSOS__GUI__GUICOM_H

/**
 * GUICom is responsable for communications to the window server
*/

#include <types.h>

namespace LIBCactusOS
{
    #define GUICOM_REQUESTCONTEXT 1

    class GUICommunication
    {
    private:
        static int windowServerID;
        static uint32_t virtualFramebufferAddress;
    public:
        /**
         * Request a context buffer for the application to draw to, this buffer is shared between the process and the window server
         * 
         * returns the virtual address of the framebuffer
         * @param width The width of the context
         * @param height The height of the context
        */
        static uint32_t RequestContext(int width, int height, int x, int y);
    };
}

#endif