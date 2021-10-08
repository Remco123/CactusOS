#ifndef __COMPISTOR_H
#define __COMPISTOR_H

#include <types.h>
#include <gui/rect.h>
#include <gui/canvas.h>
#include <ipc.h>
#include <imaging/image.h>
#include "contextmanager.h"
#include "debugger.h"

#define COMPOSITOR_DEFAULT_BACKGROUND 0xFFBFFFD0

class CompositorDebugger;

// Main class of the compositor software
class Compositor
{
friend class CompositorDebugger;
public:
#pragma region Mouse Specifics

    /**
     * Mouse current X position
    */
    int32_t curMouseX = -1;
    /**
     * Mouse current Y position
    */
    int32_t curMouseY = -1;
    /**
     * Mouse current Z position
    */
    int32_t curMouseZ = -1;

    /**
     * The previous X position of the mouse
    */
    int32_t prevMouseX = -1;
    /**
     * The previous Y position of the mouse
    */
    int32_t prevMouseY = -1;
    /**
     * The previous Z position of the mouse
    */
    int32_t prevMouseZ = -1;

protected:
    /**
     * Holds if the left mouse button was previously pressed
    */
    bool prevMouseLeft = false;
    /**
     * Holds if the right mouse button was previously pressed
    */
    bool prevMouseRight = false;
    /**
     * Holds if the middle mouse button was previously pressed
    */
    bool prevMouseMiddle = false;

#pragma endregion
#pragma region Buffer Pointers

    /**
     * The double buffer used for drawing frames
     * First all contexts are drawn to this buffer, then the buffer gets pushed to the framebuffer
    */
    uint8_t* backBuffer = 0;

    /**
     * A buffer that stores the current background
     * The size of this buffer is -> Width * Height * 4
    */
    uint8_t* backgroundBuffer = 0;

    /**
     * Image class of the background
    */
    LIBCactusOS::Imaging::Image* backgroundImage = 0;

#pragma endregion
#pragma region Canvasses

    /**
     * A canvas that can be used to manipulate the backbuffer
    */

    Canvas* backBufferCanvas;
    /**
     * A canvas that can be used to manipulate the background buffer
    */
    Canvas* backgroundCanvas = 0;

#pragma endregion

    // The contextmanager we are using for this compositor
    ContextManager* contextManager = 0;

    // The debugger used by this compositor
    CompositorDebugger* debugger = 0;
    
    // List of dirty rectangles
    List<Rectangle> dirtyRectList;

    /**
     * Which ID does the next context get on creation? 
    */
    int nextContextID = 1;

protected:
    // Makes rectangle fit into desktop rectangle
    void ApplyDesktopBounds(Rectangle* rect);

    // Draws the current cursor to the backbuffer
    void DrawCursor();

    // Removes the previous cursor still visible in the backbuffer
    void RemovePreviousCursor();

    // Function that handles a request from a client
    void HandleClientRequest(IPCMessage message);
public:
    // Create a new instance of a compositor, there should be only 1 at a time. (Propably never more than the default one though)
    Compositor();

    // Deconstructor used to free memory
    ~Compositor();

    // Draw a new frame of the desktop to the framebuffer
    void DrawFrame();

    // Process all the events
    // This includes keypresses and mouse movement/clicks
    void ProcessEvents();

    // Process all the requests received via IPC.
    // This handles things like context creation, resizing, movement and more.
    void ProcessRequests();
};

#endif