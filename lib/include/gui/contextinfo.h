#ifndef CONTEXTINFO_H
#define CONTEXTINFO_H

#include <types.h>
#include <proc.h>
#include <gui/rect.h>

// We reserve 1 page for the context info structure for alignment reasons
// The struct itself is not actually this big though
#define CONTEXT_INFO_SIZE 4_KB

// Maximum of dirty rects per frame per contextinfo struct
#define CONTEXT_INFO_MAX_DIRTY 200

// Directions a context can be resized in
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

    // Each context gets it own unique id, this way the compositor can find the right context for each message. For example when a keypress occurs.
    int id;

    // Does this context support the dirty rectangle technique?
    bool supportsDirtyRects;

    // Spinlock for modifying dirty rectangles
    bool dirtyLockFlag;

    // Number of dirty rectangles in the array below
    LIBCactusOS::uint16_t numDirtyRects;

    // Array of dirty rectangles specific to this context
    struct
    {
        // The width of this rectangle
        int width;
        // The height of this rectangle
        int height;
        // The x coördinate of this rectangle
        int x;
        // The y coördinate of this rectangle
        int y;
    } dirtyRects[CONTEXT_INFO_MAX_DIRTY];

    // Mark an area as dirty so that the compositor draws it
    void AddDirtyArea(int x, int y, int width, int height)
    {
        // Wait until no one else is also doing this
        while(dirtyLockFlag) Process::Yield();

        if(this->numDirtyRects >= CONTEXT_INFO_MAX_DIRTY)
            return; // Skip this one since the array is full :(

        // Now we take control
        dirtyLockFlag = true;

        // Add dirty rectangle
        this->dirtyRects[this->numDirtyRects].x = x;
        this->dirtyRects[this->numDirtyRects].y = y;
        this->dirtyRects[this->numDirtyRects].width = width;
        this->dirtyRects[this->numDirtyRects].height = height;
        this->numDirtyRects += 1;

        // Release lock
        dirtyLockFlag = false;
    }

    // Mark an area as dirty so that the compositor draws it
    void AddDirtyArea(Rectangle* rect) { this->AddDirtyArea(rect->x, rect->y, rect->width, rect->height); }
};

// Check if the structure doesn't cross page boundary
STATIC_ASSERT(sizeof(ContextInfo) < CONTEXT_INFO_SIZE);

#endif