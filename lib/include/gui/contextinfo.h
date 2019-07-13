#ifndef CONTEXTINFO_H
#define CONTEXTINFO_H

#include <types.h>

struct ContextInfo
{
    // To which memory address is the framebuffer mapped on the server side
    LIBCactusOS::uint32_t virtAddrServer;
    // To which memory address is the framebuffer mapped on the client side
    LIBCactusOS::uint32_t virtAddrClient;
    // How many bytes does this context use?
    LIBCactusOS::uint32_t bytes;
    // The width of this context
    LIBCactusOS::uint32_t width;
    // The height of this context
    LIBCactusOS::uint32_t height;
    // The position on the horizontal axis
    LIBCactusOS::int32_t x;
    // The position on the vertical axis
    LIBCactusOS::int32_t y;
    // The PID of the process that created this context
    int clientID;
    // Does this context support transparency? If so it will be drawn using an alternative method.
    // Warning: If set to true the drawing will be slower.
    bool supportsTransparency;
    // Can this context be moved to the front using a mouse click in it? (this will be done automatically when set to false)
    bool background;
};

#endif