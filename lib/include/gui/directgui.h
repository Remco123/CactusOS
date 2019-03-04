#ifndef __CACTUSOSLIB__GUI__DIRECTGUI_H
#define __CACTUSOSLIB__GUI__DIRECTGUI_H

namespace LIBCactusOS
{
    #define DIRECT_GUI_ADDR 0xC0000000 - (0x8000 + 0x4000 * 0x400) //Just below the userstack minus the maximum size of the framebuffer (16384 Kb)

    class DirectGUI
    {
    public:
        static bool RequestFramebuffer();
    };
}

#endif