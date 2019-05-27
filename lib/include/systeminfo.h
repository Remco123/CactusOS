#ifndef __LIBCACTUSOS__SYSTEMINFO_H
#define __LIBCACTUSOS__SYSTEMINFO_H

namespace LIBCactusOS
{
    /**
     * This struct can be shared between the kernel and userspace processes
    */
    struct SharedSystemInfo
    {
        unsigned int MouseX;
        unsigned int MouseY;
        signed char MouseZ;

        bool MouseLeftButton;
        bool MouseRightButton;
        bool MouseMiddleButton;
    } __attribute__((packed));

    bool RequestSystemInfo(unsigned int mapTo = 0xBA000000);
}

#endif