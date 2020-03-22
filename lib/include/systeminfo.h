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

    #define SYSTEM_INFO_ADDR 0xBFFEE000

    // Request System info to be mapped into adress space
    // Addres is 0xBFFEE000 which is 2 pages below user-stack
    bool RequestSystemInfo();
}

#endif