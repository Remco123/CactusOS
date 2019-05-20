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
    public:
        /**
         * Request a context buffer for the application to draw to, this buffer is shared between the process and the window server
         * 
         * @param virtAddr The virtual addres to which the buffer is mapped to
         * @param width The width of the context
         * @param height The height of the context
        */
        static bool RequestContext(uint32_t virtAddr, int width, int height, int x, int y);
    };
}

#endif