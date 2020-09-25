#ifndef CONTEXTINFO_H
#define CONTEXTINFO_H

#include <types.h>

//Directions a context can be resized in
enum Direction
{
    None = (0<<0),
    Top = (1<<0),
    Right = (1<<1),
    Bottom = (1<<2),
    Left = (1<<3)
};

inline Direction operator|(Direction a, Direction b)
{
    return static_cast<Direction>(static_cast<int>(a) | static_cast<int>(b));
}

struct ContextInfo
{
    // To which memory address is the framebuffer mapped on the server side (ContextInfo is placed just before this address)
    LIBCactusOS::uint32_t virtAddrServer;
    // To which memory address is the framebuffer mapped on the client side (ContextInfo is placed just before this address)
    LIBCactusOS::uint32_t virtAddrClient;

    // How many bytes does this context use? This includes the memory used by this struct
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

    // Each context gets it own uniqe id, this way the compositor can find the right context for each message. For example when a keypress occurs.
    int id;
};

#endif