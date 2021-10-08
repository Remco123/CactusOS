#include "compositor.h"
#include "cursor.h"

#include <gui/directgui.h>
#include <gui/gui.h>
#include <gui/colors.h>
#include <imaging/image.h>
#include <systeminfo.h>
#include <proc.h>
#include <gui/contextheap.h>
#include <log.h>
#include <ipc.h>
#include <math.h>
#include <string.h>
#include <heap.h>

using namespace LIBCactusOS;
using namespace LIBCactusOS::Imaging;

extern uint8_t ConvertKeycode(KeypressPacket* packet); //In scancodes.cpp

Compositor::Compositor()
: dirtyRectList()
{
    GUI::SetDefaultFont();
    
    Print("[Compositor] Starting Compositor\n");
    if(!DirectGUI::RequestFramebuffer()) {
        Print("[Compositor] Error initializing framebuffer\n");
        return;
    }
    Print("[Compositor] Direct Framebuffer Initialized\n");

    Print("[Compositor] Starting Context Manager\n");
    this->contextManager = new ContextManager();

    Print("[Compositor] Loading debugger\n");
    this->debugger = new CompositorDebugger(this);

    Print("[Compositor] Allocating Backbuffer of %f Mb\n", (double)(GUI::Width*GUI::Height*4) / (double)1_MB);
    this->backBuffer = new uint8_t[GUI::Width*GUI::Height*4];
    this->backBufferCanvas = new Canvas(backBuffer, GUI::Width, GUI::Height);

    DirectGUI::DrawString("Loading Background from disk....", 3, 3, 0xFF000000);
    Image* orgBackground = Image::CreateFromFile("B:\\wallpap.jpg");

    DirectGUI::DrawString("Resizing Background...", 3, 16, 0xFF000000);
    this->backgroundImage = Image::Resize(orgBackground, GUI::Width, GUI::Height, Bilinear);
    if(this->backgroundImage == 0) // Error with loading background
    {
        Print("[Compositor] Creating custom background\n");
        this->backgroundImage = new Image(GUI::Width, GUI::Height);
        this->backgroundImage->GetCanvas()->Clear(COMPOSITOR_DEFAULT_BACKGROUND);

        if(orgBackground) delete orgBackground;
    }
    
    // Point buffer and canvas to this resized image we use as the background
    this->backgroundBuffer = (uint8_t*)(this->backgroundImage->GetBufferPtr());
    this->backgroundCanvas = this->backgroundImage->GetCanvas();

    // Copy background to backbuffer, otherwise it contains noise at the start
    memcpy(this->backBuffer, this->backgroundBuffer, GUI::Width*GUI::Height*4);

    Print("[Compositor] Requesting Systeminfo\n");
    if(!SystemInfo::RequestSystemInfo())
        return;

    Print("[Compositor] Requesting direct keyboard input\n");
    Process::BindSTDIO(-1, Process::ID);

    Print("[Compositor] Preparing Context Memory allocation\n");
    ContextHeap::Init();

    Print("[Compositor] Compositor initialized\n");
}

Compositor::~Compositor()
{
    Print("[Compositor] Freeing up memory used by compositor\n");

    if(this->backBuffer)
        delete this->backBuffer;
    
    if(this->backBufferCanvas)
        delete this->backBufferCanvas;
    
    if(this->backgroundBuffer)
        delete this->backgroundBuffer;
    
    if(this->backgroundCanvas)
        delete this->backgroundCanvas;
    
    if(this->contextManager)
        delete this->contextManager;
}

void Compositor::ApplyDesktopBounds(Rectangle* rect)
{
    if(rect->x < 0) {
        rect->width -= Math::Abs(rect->x);
        rect->x = 0;
    }
    if(rect->y < 0) {
        rect->height -= Math::Abs(rect->y);
        rect->y = 0;
    }
    if((rect->x + rect->width) >= GUI::Width) {
        rect->width = GUI::Width - rect->x;
    }
    if((rect->y + rect->height) >= GUI::Height) {
        rect->height = GUI::Height - rect->y;
    }
}

void Compositor::DrawCursor()
{
    // Calculate the boundary for the cursor
    uint8_t x_d = this->curMouseX + CURSOR_W < GUI::Width  ? CURSOR_W : GUI::Width  - this->curMouseX;
    uint8_t y_d = this->curMouseY + CURSOR_H < GUI::Height ? CURSOR_H : GUI::Height - this->curMouseY;

    // Loop through all the individual cursor pixels
    for(uint8_t x = 0; x < x_d; x++)
        for(uint8_t y = 0; y < y_d; y++) {
            uint8_t cd = cursorBitmap[y*CURSOR_W+x];
            if(cd != ALPHA) // Don't draw it if this pixel should be transparent
                this->backBufferCanvas->SetPixel(this->curMouseX + x, this->curMouseY + y, cd==WHITE ? 0xFFFFFFFF : 0xFF000000);
        }
}

// Instead of this function it is also possible to simply add a dirty rectangle for the cursor.
// But the speed impact is so little that we don't really care about this.
void Compositor::RemovePreviousCursor()
{
    // How much of the previous cursor should be removed?
    // These values will be smaller than the cursor width when the mouse is partially in the corner
    uint8_t x_d = this->prevMouseX + CURSOR_W < GUI::Width  ? CURSOR_W : GUI::Width  - this->prevMouseX;
    uint8_t y_d = this->prevMouseY + CURSOR_H < GUI::Height ? CURSOR_H : GUI::Height - this->prevMouseY;

    for(uint8_t x = 0; x < x_d; x++)
        for(uint8_t y = 0; y < y_d; y++)
            this->backBufferCanvas->SetPixel(this->prevMouseX + x, this->prevMouseY + y, this->backgroundCanvas->GetPixel(this->prevMouseX + x, this->prevMouseY + y));
}

void Compositor::DrawFrame()
{
    // Check if we have valid values for prevMouseX/Y and check if the mouse has moved
    if(this->prevMouseX != -1 && this->prevMouseY != -1 && (this->prevMouseX != this->curMouseX || this->prevMouseY != this->curMouseY))
        RemovePreviousCursor();

    // Update dirty rectangles by drawing the background image at every dirty rect position
    for(Rectangle rect : dirtyRectList) 
    {
        //Print("[Compositor] Dirty Rectangle (%d,%d,%d,%d)\n", rect.x, rect.y, rect.width, rect.height);

        ApplyDesktopBounds(&rect);
        uint32_t byteWidth = (rect.width + rect.x <= GUI::Width ? rect.width : rect.width-(rect.x + rect.width - GUI::Width))*4;
        for(uint32_t y = 0; y < (uint32_t)rect.height; y++)
            memcpy((void*)(backBuffer + ((rect.y + y)*GUI::Width*4) + rect.x*4), (void*)((uint32_t)this->backgroundBuffer + (rect.y + y)*GUI::Width*4 + rect.x*4), byteWidth);
    }
    dirtyRectList.Clear(); // After processing all the dirty rects for this frame, we can clear the list.

    // Draw every context bottom to top since the contextList is organized that way
    for(int i = (this->contextManager->contextList.size()-1); i >= 0; i--)
    {
        ContextInfo* context = this->contextManager->contextList[i];
        if(context->x >= GUI::Width || context->y >= GUI::Height) {
            Print("[Compositor] Warning! Context %d is out of desktop bounds\n", i);
            continue; // Skip this context since it will not be visible anyway
        }

        // Context does not support dirty rectangles
        if(!context->supportsDirtyRects) {
            // Create rectangle for this context for easy calculations
            Rectangle contextRectangle = Rectangle(context->width, context->height, context->x, context->y);
            ApplyDesktopBounds(&contextRectangle);
            
            uint32_t leftOffset = ((context->x < 0) ? -context->x : 0);
            uint32_t topOffset = ((context->y < 0) ? -context->y : 0);

            // Check if context needs to be drawn using the transparency method
            if(context->supportsTransparency) {
                for(int y = 0; y < contextRectangle.height; y++)
                    for(int x = 0; x < contextRectangle.width; x++)
                        *(uint32_t*)(this->backBuffer + (contextRectangle.y + y)*GUI::Width*4 + (contextRectangle.x + x)*4) = Colors::AlphaBlend(*(uint32_t*)(this->backgroundBuffer + (contextRectangle.y + y)*GUI::Width*4 + (contextRectangle.x + x)*4), *(uint32_t*)(context->virtAddrServer + (topOffset+y)*context->width*4 + (leftOffset+x)*4));
            }
            else { // Otherwise we use the very optimized way of drawing
                for(int hOffset = 0; hOffset < contextRectangle.height; hOffset++)
                    memcpy((this->backBuffer + (contextRectangle.y+hOffset)*GUI::Width*4 + contextRectangle.x*4), (void*)(context->virtAddrServer + leftOffset*4 + (topOffset + hOffset)*context->width*4), contextRectangle.width * 4);
            }
        }
        else if(context->numDirtyRects > 0)
        {
            //Log(Info, "[Compositor] Drawing dirty rectangles for client");
            for(int dirtyIndex = 0; dirtyIndex < context->numDirtyRects; dirtyIndex++)
            {
                // Create rectangle for this context for easy calculations
                Rectangle contextRectangle = Rectangle(context->dirtyRects[dirtyIndex].width, context->dirtyRects[dirtyIndex].height, context->dirtyRects[dirtyIndex].x + context->x, context->dirtyRects[dirtyIndex].y + context->y);
                ApplyDesktopBounds(&contextRectangle);
                
                uint32_t leftOffset = (context->dirtyRects[dirtyIndex].x);
                uint32_t topOffset = (context->dirtyRects[dirtyIndex].y);

                // Check if context needs to be drawn using the transparency method
                if(context->supportsTransparency) {
                    for(int y = 0; y < contextRectangle.height; y++)
                        for(int x = 0; x < contextRectangle.width; x++)
                            *(uint32_t*)(this->backBuffer + (contextRectangle.y + y)*GUI::Width*4 + (contextRectangle.x + x)*4) = Colors::AlphaBlend(*(uint32_t*)(this->backgroundBuffer + (contextRectangle.y + y)*GUI::Width*4 + (contextRectangle.x + x)*4), *(uint32_t*)(context->virtAddrServer + (topOffset+y)*context->width*4 + (leftOffset+x)*4));
                }
                else { // Otherwise we use the very optimized way of drawing
                    for(int hOffset = 0; hOffset < contextRectangle.height; hOffset++)
                        memcpy((this->backBuffer + (contextRectangle.y+hOffset)*GUI::Width*4 + contextRectangle.x*4), (void*)(context->virtAddrServer + leftOffset*4 + (topOffset + hOffset)*context->width*4), contextRectangle.width * 4);
                }
            }
            context->numDirtyRects = 0;
        }

        // Draw debug info for this context when we should
        if(this->debugger->enabled)
            this->debugger->ProcessContext(context);
    }
    // Draw debug info when enabled
    if(this->debugger->enabled)
        this->debugger->ProcessGeneral();

    // Finally draw the cursor to the backbuffer
    DrawCursor();

    // Swap buffers, after this the frame is visible on the framebuffer
    memcpy(DirectGUI::GetCanvas()->bufferPointer, this->backBuffer, GUI::Width*GUI::Height*4);
}

void Compositor::ProcessEvents()
{
    bool mouseLeft = Process::systemInfo->MouseLeftButton;
    bool mouseRight = Process::systemInfo->MouseRightButton;
    bool mouseMiddle = Process::systemInfo->MouseMiddleButton;

    // MouseButton state changed
    if(mouseLeft != this->prevMouseLeft || mouseRight != this->prevMouseRight || mouseMiddle != this->prevMouseMiddle)
    {
        // Get context that is below the cursor
        ContextInfo* info = this->contextManager->FindTargetContext(this->curMouseX, this->curMouseY);

        uint8_t changedButton;
        if(mouseLeft != this->prevMouseLeft)
            changedButton = 0;
        else if(mouseMiddle != this->prevMouseMiddle)
            changedButton = 1;
        else
            changedButton = 2;

        if(info != 0) {
            // Check if the mouse has been held down or up
            bool mouseDown = changedButton == 0 ? mouseLeft : (changedButton == 1 ? mouseMiddle : (changedButton == 2 ? mouseRight : 0));
            IPCSend(info->clientID, IPCMessageType::GUIEvent, mouseDown ? GUIEvents::MouseDown : GUIEvents::MouseUp, this->curMouseX, this->curMouseY, changedButton);

            // If the mouse is held down on this context and it is not supposed to be in the background
            // Then we move the context to the front
            if(mouseDown && !info->background)
                this->contextManager->MoveToFront(info);
        }
    }

    // Mouse has moved
    if(this->curMouseX != this->prevMouseX || this->curMouseY != this->prevMouseY)
    {
        // Which context was under the previous mouse
        ContextInfo* prevMouseInfo = this->contextManager->FindTargetContext(this->prevMouseX, this->prevMouseY);

        // Which context is under the current mouse
        ContextInfo* curMouseInfo = this->contextManager->FindTargetContext(this->curMouseX, this->curMouseY);
        
        if(prevMouseInfo != 0) {
            IPCSend(prevMouseInfo->clientID, IPCMessageType::GUIEvent, GUIEvents::MouseMove, this->prevMouseX, this->prevMouseY, this->curMouseX, this->curMouseY);
        }
        if(curMouseInfo != 0 && curMouseInfo != prevMouseInfo) {
            IPCSend(curMouseInfo->clientID, IPCMessageType::GUIEvent, GUIEvents::MouseMove, this->prevMouseX, this->prevMouseY, this->curMouseX, this->curMouseY);
        }

        // Apply dirty rectangles if required
        Rectangle prevMouseRect = Rectangle(CURSOR_W, CURSOR_H, this->prevMouseX, this->prevMouseY);
        Rectangle curMouseRect = Rectangle(CURSOR_W, CURSOR_H, this->curMouseX, this->curMouseY);
        ApplyDesktopBounds(&prevMouseRect);
        ApplyDesktopBounds(&curMouseRect);

        for(ContextInfo* c : contextManager->FindTargetContexts(prevMouseRect))
            c->AddDirtyArea(0, 0, c->width, c->height);

        for(ContextInfo* c : contextManager->FindTargetContexts(curMouseRect))
            c->AddDirtyArea(0, 0, c->width, c->height);
    }

    // Mouse scroll wheel position has changed
    if(this->curMouseZ != this->prevMouseZ)
    {
        // Calculate difference
        int32_t delta = this->curMouseZ - this->prevMouseZ;

        // Get context that is below the cursor
        ContextInfo* info = this->contextManager->FindTargetContext(this->curMouseX, this->curMouseY);

        // Send event to context
        if(info)
            IPCSend(info->clientID, IPCMessageType::GUIEvent, GUIEvents::MouseScroll, info->id, delta, this->curMouseX, this->curMouseY);
    }

    // Update variables for next iteration
    this->prevMouseLeft     = mouseLeft;
    this->prevMouseMiddle   = mouseMiddle;
    this->prevMouseRight    = mouseRight;

    // Process all the pressed keys
    while(Process::StdInAvailable() > 0)
    {
        uint8_t startByte = Process::ReadStdIn();
        if(startByte != KEYPACKET_START) {
            Print("[Compositor] Warning! Some noise on Compositor standard input\n");
            continue;
        }
        
        KeypressPacket packet;
        memset(&packet, 0, sizeof(KeypressPacket));
        // i = 1 for skipping start byte
        for(int i = 1; i < (int)sizeof(KeypressPacket); i++)
            *(uint8_t*)((uint32_t)&packet + i) = Process::ReadStdIn();
        
        uint8_t key = ConvertKeycode(&packet);
        if(key == 0) {
            Print("[Compositor] No key for scancode %b\n", packet.keyCode);
            continue;
        }

        // Debug mode key (Ctrl + d)
        if(key == 'd' && (packet.flags & LeftControl) && (packet.flags & Pressed)) {
            Print("[Compositor] Switching debug mode to %b\n", !this->debugger->enabled);
            this->debugger->enabled = !this->debugger->enabled;

            if(this->debugger->enabled == false) // Cleanup mess left by debugger
                this->dirtyRectList.push_back(Rectangle(GUI::Width, GUI::Height));

            continue;
        }

        // No context present to send the key to
        if(this->contextManager->contextList.size() == 0)
            continue; // Use continue instead of break so that all keys will get processed and not remain in the buffer
        
        ContextInfo* sendTo = this->contextManager->contextList.GetAt(0); //Send key to the context currently in focus
        IPCSend(sendTo->clientID, IPCMessageType::GUIEvent, GUIEvents::Keypress, (uint32_t)key, (uint32_t)packet.flags, (uint32_t)sendTo->id);
    }
}

void Compositor::ProcessRequests()
{
    // Check if there is a message that needs to be handled
    while(IPCAvailable() > 0)
    {
        int msgError = 0;
        IPCMessage message;

        // Receive message via IPC bus
        message = ICPReceive(-1, &msgError, IPCMessageType::GUIRequest);

        // Check if message is received properly
        if(msgError == SYSCALL_RET_ERROR) {
            Print("[Compositor] Warning! Something wrong with message, ignoring\n");
            return;
        }

        // Call the function that handles all the possible events
        this->HandleClientRequest(message);
    }
}

void Compositor::HandleClientRequest(IPCMessage msg)
{
    //Print("[Compositor] Received request from %d ---> type = %d\n", msg.source, msg.arg1);

    switch (msg.arg1)
    {
        // A process is requesting a new context to draw to
        case GUICommunction::REQUEST_CONTEXT:
        {
            // Gather parameters from message
            uint32_t width = msg.arg3;
            uint32_t height = msg.arg4;
            uint32_t x = msg.arg5;
            uint32_t y = msg.arg6;

            // Address of memory on the client side
            uint32_t memAddressClient = msg.arg2;

            // Calculate the required bytes needed for this context
            uint32_t bytesRequired = (width * height * 4) + CONTEXT_INFO_SIZE;

            // Address of memory on the server side (our side)
            uint32_t contextAddress = ContextHeap::AllocateArea(pageRoundUp(bytesRequired) / 4_KB);
            Print("[Compositor] Process %d requested a context of %d bytes at %x (w=%d,h=%d,x=%d,y=%d) mapping to %x\n", msg.source, bytesRequired, memAddressClient, width, height, x, y, contextAddress);
            
            // Map this memory between us and the client process
            // Virtual memory is managed in blocks of 4Kb so it must be page aligned
            if(Process::CreateSharedMemory(msg.source, contextAddress, memAddressClient, pageRoundUp(bytesRequired)) == false) {
                Print("[Compositor] Error creating shared memory\n");
                IPCSend(msg.source, IPCMessageType::GUIRequest, 0);
                break;
            }

            // Create pointer to struct that we want to fill with info about this context
            ContextInfo* info = (ContextInfo*)contextAddress;

            // Make sure it is completely zeroed
            memset(info, 0, CONTEXT_INFO_SIZE);

            // Start filling in the info
            info->bytes = bytesRequired;
            info->virtAddrClient = memAddressClient + CONTEXT_INFO_SIZE;
            info->virtAddrServer = contextAddress + CONTEXT_INFO_SIZE;
            info->width = width;
            info->height = height;
            info->x = x;
            info->y = y;
            info->clientID = msg.source;
            info->supportsTransparency = false;
            info->background = false;
            info->id = nextContextID++;
            info->numDirtyRects = 1;

            // Add this context to our list of all contexts
            this->contextManager->contextList.push_front(info);

            // Send response to client
            IPCSend(msg.source, IPCMessageType::GUIRequest, 1);
            break;
        }
        // A process is sending us a message that one of its contexts has moved
        case GUICommunction::CONTEXT_MOVED:
        {
            Rectangle dirtyRect(msg.arg4, msg.arg5, msg.arg2, msg.arg3);
            this->dirtyRectList.push_back(dirtyRect);
            break;
        }
        // A process requested a close of context
        case GUICommunction::REQUEST_CLOSE:
        {
            int contextID = msg.arg2;
            for(int i = 0; i < this->contextManager->contextList.size(); i++)
            {
                ContextInfo* c = this->contextManager->contextList[i];
                if(c == 0)
                    continue;

                if(c->id == contextID)
                {
                    Print("[Compositor] Removing context: %x\n", (uint32_t)c);
                    this->contextManager->contextList.Remove(c);

                    // Add a dirty rect at the old position of the context
                    Rectangle dirtyRect(c->width, c->height, c->x, c->y);
                    this->dirtyRectList.push_back(dirtyRect);

                    // Free area of virtual allocated memory
                    ContextHeap::FreeArea(c->virtAddrServer + CONTEXT_INFO_SIZE, pageRoundUp(c->bytes) / 4_KB);
                    
                    // Free shared memory
                    if(!Process::DeleteSharedMemory(c->clientID, c->virtAddrServer - CONTEXT_INFO_SIZE, c->virtAddrClient - CONTEXT_INFO_SIZE, pageRoundUp(c->bytes)))
                        Print("[Compositor] Error! Could not remove shared memory\n");
                }
            }
            IPCSend(msg.source, IPCMessageType::GUIRequest, 1);
            break;
        }

        default:
        {
            Print("[Compositor] Error! Got unknown type of message from %d type = %d\n", msg.source, msg.arg1);
            break;
        }
    }
}