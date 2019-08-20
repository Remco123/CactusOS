#include <new.h>
#include <log.h>
#include <gui/directgui.h>
#include <ipc.h>
#include <syscall.h>
#include <gui/canvas.h>
#include <gui/gui.h>
#include <gui/colors.h>
#include <list.h>
#include <time.h>
#include <proc.h>
#include <string.h>
#include <systeminfo.h>
#include <gui/contextinfo.h>
#include "cursor.h"

using namespace LIBCactusOS;

//////////////
// Functions
//////////////
void HandleMessage(IPCMessage msg);
void UpdateDesktop();
void RemovePreviousCursor();
void DrawCursor();
void ProcessEvents();
extern uint8_t* LoadBackground(char*); //In background.cpp

//////////
// Holds current mouse positions
/////////
int32_t curMouseX = -1;
int32_t curMouseY = -1;

/**
 * All the known contexts
*/
List<ContextInfo*>* contextList;
/**
 * All the rectangles that need to be redrawn on the next iteration
*/
List<Rectangle>* dirtyRectList;
/**
 * To wich address are context mapped in virtual memory?
*/
uint32_t newContextAddress = 0xA0000000;
/**
 * The double buffer
*/
uint8_t* backBuffer = 0;
/**
 * A canvas that can be used to manipulate the backbuffer
*/
Canvas* backBufferCanvas;
/**
 * A buffer that stores the current wallpaper
*/
uint8_t* wallPaperBuffer = 0;
/**
 * A canvas that can be used to manipulate the wallpaper buffer
*/
Canvas* wallPaperCanvas = 0;
/**
 * The previous x position of the mouse
*/
int prevMouseX = -1;
/**
 * The previous y position of the mouse
*/
int prevMouseY = -1;
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
/**
 * Which ID does the next context get on creation? 
*/
int nextContextID = 1;

void GUILoop()
{
    Print("GUI loop started\n");

    //Copy background to backbuffer
    if(wallPaperBuffer != 0)
        memcpy((void*)backBuffer, (void*)wallPaperBuffer, WIDTH*HEIGHT*4);
    
    while(true) {
        ////////
        // Update mouse positions
        ////////
        curMouseX = Process::systemInfo->MouseX;
        curMouseY = Process::systemInfo->MouseY;

        ////////
        // Process GUI Events
        ////////
        ProcessEvents();

        ////////
        // Draw a new version of the desktop
        ////////
        UpdateDesktop();
    }
}

ContextInfo* FindTargetContext(int x, int y)
{
    if(contextList == 0)
        return 0;
    
    for(int i = 0; i < contextList->size(); i++)
    {
        ContextInfo* c = contextList->GetAt(i);

        if(x >= c->x && x <= c->x + c->width)
            if(y >= c->y && y <= c->y + c->height)
                return c;
    }
    return 0;
}

ContextInfo* FindTargetContextByIndex(int sourcePID, int index)
{
    if(contextList == 0)
        return 0;
    
    int curIndex = 0;
    for(int i = 0; i < contextList->size(); i++)
    {
        ContextInfo* c = contextList->GetAt(i);

        if(c->clientID == sourcePID && curIndex == index)
            return c;
        else
            curIndex++;
    }
    return 0;
}

int main()
{
    Print("Starting Compositor\n");
    if(!DirectGUI::RequestFramebuffer()) {
        Log(Error, "Error initializing framebuffer");
        return -1;
    }
    Print("Framebuffer Initialized\n");

    Print("Allocating Backbuffer\n");
    backBuffer = new uint8_t[WIDTH*HEIGHT*4];
    backBufferCanvas = new Canvas(backBuffer, WIDTH, HEIGHT);

    DirectGUI::DrawString("Loading Background...", 3, 3, 0xFF000000);
    Print("Loading Background\n");
    wallPaperBuffer = LoadBackground("B:\\wallpap.bmp");
    wallPaperCanvas = new Canvas(wallPaperBuffer, WIDTH, HEIGHT);

    Print("Requesting Systeminfo\n");
    if(!RequestSystemInfo())
        return -1;

    Print("Requesting direct keyboard input\n");
    Process::BindSTDIO(-1, Process::ID);

    contextList = new List<ContextInfo*>(); contextList->Clear();
    dirtyRectList = new List<Rectangle>(); dirtyRectList->Clear();
    Print("Listening for requests\n");
    
    bool receivedMessage = false;
    while (1)
    {        
        int msgError = 0;
        IPCMessage msg = ICPReceive(-1, &msgError, IPC_TYPE_GUI);

        if(msgError == SYSCALL_RET_ERROR || msg.type != IPC_TYPE_GUI) {
            Print("Something wrong with message, ignoring\n");
            continue;
        }

        //Print("Got Request from %d\n", msg.source);
        HandleMessage(msg);

        if(!receivedMessage) //First time we receive something start the GUI thread
        {
            receivedMessage = true;
            Print("Creating GUI Loop\n");
            Process::CreateThread(GUILoop, false);
        }
    }

    return 0;
}

void HandleMessage(IPCMessage msg)
{
    int msgType = msg.arg1;
    switch (msgType)
    {
        // A process is requesting a new context to draw to
        case COMPOSITOR_REQUESTCONTEXT:
        {
            uint32_t width = msg.arg3;
            uint32_t height = msg.arg4;
            uint32_t x = msg.arg5;
            uint32_t y = msg.arg6;

            uint32_t bytes = width * height * 4 + sizeof(ContextInfo);
            uint32_t virtAddrC = msg.arg2;
            Print("Process %d requested a gui context of %d bytes at %x (w=%d,h=%d,x=%d,y=%d)\n", msg.source, bytes, virtAddrC, width, height, x, y);
            if(Process::CreateSharedMemory(msg.source, newContextAddress, virtAddrC, pageRoundUp(bytes)) == false) {
                Print("Error creating shared memory\n");
                break;
            }

            ContextInfo* info = (ContextInfo*)newContextAddress;
            info->bytes = bytes;
            info->virtAddrClient = virtAddrC + sizeof(ContextInfo);
            info->virtAddrServer = newContextAddress + sizeof(ContextInfo);
            info->width = width;
            info->height = height;
            info->x = x;
            info->y = y;
            info->clientID = msg.source;
            info->supportsTransparency = false;
            info->background = false;
            info->id = nextContextID++;

            newContextAddress += pageRoundUp(bytes);
            contextList->push_front(info);

            //Send response to client
            IPCSend(msg.source, IPC_TYPE_GUI, 1);
            break;
        }
        // A process is sending us a message that one of its contexts has moved
        case COMPOSITOR_CONTEXTMOVED:
        {
            Rectangle dirtyRect(msg.arg4, msg.arg5, msg.arg2, msg.arg3);
            dirtyRectList->push_back(dirtyRect);
            break;
        }
        // A process requested a close of context
        case COMPOSITOR_CONTEXTCLOSE:
        {
            int contextID = msg.arg2;
            for(int i = 0; i < contextList->size(); i++)
            {
                ContextInfo* c = contextList->GetAt(i);
                if(c == 0)
                    continue;

                if(c->id == contextID)
                {
                    Print("[Compositor] Removing context: %x\n", (uint32_t)c);
                    contextList->Remove(c);
                    
                    //Finally add a dirty rect at the old position of the context
                    Rectangle dirtyRect(c->width, c->height, c->x, c->y);
                    dirtyRectList->push_back(dirtyRect);
                }
            }
            break;
        }

        default:
        {
            Log(Warning, "Got unkown GUICom message\n");
            break;
        }
    }
}

void UpdateDesktop()
{    
    if(prevMouseX != -1 && prevMouseY != -1 && (prevMouseX != curMouseX || prevMouseY != curMouseY)) //Check if we have valid values for prevMouseX/Y and check if the mouse has moved
        RemovePreviousCursor();
    
    //Update dirty rectangles
    while(dirtyRectList->size() > 0)
    {
        Rectangle rect = dirtyRectList->GetAt(0);
        uint32_t byteWidth = (rect.width + rect.x <= WIDTH ? rect.width : rect.width-(rect.x + rect.width - WIDTH))*4;
        for(uint32_t y = 0; y < rect.height; y++)
            memcpy((void*)(backBuffer + ((rect.y + y)*WIDTH*4) + rect.x*4), (void*)((uint32_t)wallPaperBuffer + (rect.y + y)*WIDTH*4 + rect.x*4), byteWidth);

        dirtyRectList->Remove(0);
    }

    //Draw every context bottom to top
    for(int i = (contextList->size()-1); i >= 0; i--)
    {
        ContextInfo* info = contextList->GetAt(i);
        if(info->x >= WIDTH || info->y >= HEIGHT) {
            Log(Warning, "Context is out of desktop bounds");
            continue;
        }
        
        if(info->supportsTransparency) //Draw context using transparency
        {
            // Create temporary canvas for easy pixel access.
            Canvas tempCanvas((void*)info->virtAddrServer, info->width, info->height);
            
            /////////////
            // Draw context with every context in the background
            /////////////

            // Generate a rectangle in the shape of this context
            Rectangle infoRectangle(info->width, info->height, info->x, info->y);

            // Create a list that holds all the rectangles that are beneath this context
            List<Rectangle> underlayingRectangles;

            // Loop trough all the contexts that have a lower index than us
            for(int j = contextList->size(); j --> 0; )
            {
                ContextInfo* underlayingContext = contextList->GetAt(j);

                if(underlayingContext == info) // This is ourself
                    break; //Stop becouse we only want to check contexts that are beneath this context
                
                // Create rectangle for the target context
                Rectangle targetRect(underlayingContext->width, underlayingContext->height, underlayingContext->x, underlayingContext->y);
                
                // Calculate intersection
                Rectangle intersectRect(0, 0, 0, 0);
                bool intersect = infoRectangle.Intersect(targetRect, &intersectRect);
                if(intersect) // There is a intersection between this context and an other context
                {
                    underlayingRectangles.push_back(intersectRect);
                    for(int x = 0; x < intersectRect.width; x++)
                        for(int y = 0; y < intersectRect.height; y++)
                        {
                            uint32_t pix = tempCanvas.GetPixel(x + (intersectRect.x - info->x), y + (intersectRect.y - info->y));
                            uint32_t oldPix = backBufferCanvas->GetPixel(intersectRect.x + x, intersectRect.y + y);
                            backBufferCanvas->SetPixel(intersectRect.x + x, intersectRect.y + y, Colors::AlphaBlend(oldPix, pix));
                        }
                }
            }

            //////////////
            // Draw parts where only the wallpaper is behind
            //////////////
            // Which rectangles contain the wallpaper
            List<Rectangle> backgroundRects;
            backgroundRects.push_back(infoRectangle);
            for(int k = 0; k < underlayingRectangles.size(); k++) {
                underlayingRectangles[k].PushToClipList(&backgroundRects);
                backgroundRects.Remove(backgroundRects.size() - 1);
            }

            //Loop through all the rects with wallpaper behind them
            for(Rectangle r : backgroundRects)
                if(r.width > 0 && r.height > 0)
                    for(int x = 0; x < r.width; x++)
                        for(int y = 0; y < r.height; y++)
                        {
                            uint32_t pix = tempCanvas.GetPixel(x + (r.x - info->x), y + (r.y - info->y));
                            uint32_t oldPix = wallPaperCanvas->GetPixel(r.x + x, r.y + y);
                            backBufferCanvas->SetPixel(r.x + x, r.y + y, Colors::AlphaBlend(oldPix, pix));
                        }
        }
        else
        {
            //Draw context the normal way by copying the framebuffer onto the backbuffer line by line
            uint32_t byteWidth = (info->width + info->x <= WIDTH ? info->width : info->width-(info->x + info->width - WIDTH))*4;
            for(uint32_t y = (info->y < 0 ? -info->y : 0); y < info->height; y++)
                memcpy((void*)(backBuffer + ((info->y + y)*WIDTH*4) + info->x*4), (void*)(info->virtAddrServer + y*info->width*4), byteWidth);
        }
    }
    DrawCursor();

    //Swap buffers
    memcpy((void*)DIRECT_GUI_ADDR, (void*)backBuffer, WIDTH*HEIGHT*4);
}

void RemovePreviousCursor()
{
    //How much of the previous cursor should be removed?
    //These values will be smaller than the cursor width when the mouse is partialy in the corner
    uint8_t x_d = prevMouseX + CURSOR_W < WIDTH ? CURSOR_W : WIDTH - prevMouseX;
    uint8_t y_d = prevMouseY + CURSOR_H < HEIGHT ? CURSOR_H : HEIGHT - prevMouseY;

    for(uint8_t x = 0; x < x_d; x++)
        for(uint8_t y = 0; y < y_d; y++)
            backBufferCanvas->SetPixel(prevMouseX + x, prevMouseY + y, wallPaperCanvas->GetPixel(prevMouseX + x, prevMouseY + y));
}

void DrawCursor()
{
    //////////////
    // Draw new Cursor at position
    //////////////
    uint8_t x_d = curMouseX + CURSOR_W < WIDTH ? CURSOR_W : WIDTH - curMouseX;
    uint8_t y_d = curMouseY + CURSOR_H < HEIGHT ? CURSOR_H : HEIGHT - curMouseY;

    for(uint8_t x = 0; x < x_d; x++)
        for(uint8_t y = 0; y < y_d; y++) {
            uint8_t cd = cursorBitmap[y*CURSOR_W+x];
            if(cd != ALPHA)
                backBufferCanvas->SetPixel(curMouseX + x, curMouseY + y, cd==WHITE ? 0xFFFFFFFF : 0xFF000000);
        }

    ///////////////
    // Update old mouse positions
    ///////////////
    prevMouseX = curMouseX;
    prevMouseY = curMouseY;
}

void ProcessEvents()
{
    bool mouseLeft = Process::systemInfo->MouseLeftButton;
    bool mouseRight = Process::systemInfo->MouseRightButton;
    bool mouseMiddle = Process::systemInfo->MouseMiddleButton;

    ////////////
    // MouseButton state changed
    ////////////
    if(mouseLeft!=prevMouseLeft || mouseRight!=prevMouseRight || mouseMiddle!=prevMouseMiddle)
    {
        ContextInfo* info = FindTargetContext(curMouseX, curMouseY);
        if(info != 0) {
            uint8_t changedButton;
            if(mouseLeft!=prevMouseLeft)
                changedButton = 0;
            else if(mouseMiddle!=prevMouseMiddle)
                changedButton = 1;
            else
                changedButton = 2;

            //Check if the mouse has been held down or up
            bool mouseDown = changedButton == 0 ? mouseLeft : (changedButton == 1 ? mouseMiddle : (changedButton == 2 ? mouseRight : 0));
            IPCSend(info->clientID, IPC_TYPE_GUI_EVENT, mouseDown ? EVENT_TYPE_MOUSEDOWN : EVENT_TYPE_MOUSEUP, curMouseX, curMouseY, changedButton);

            if(mouseDown && !info->background)
            {
                //Move window to the front
                contextList->Remove(info);
                contextList->push_front(info);
            }
        }
    }

    ////////////
    // Mouse has moved
    ////////////
    if(curMouseX != prevMouseX || curMouseY != prevMouseY)
    {
        //Which context was under the previous mouse
        ContextInfo* prevMouseInfo = FindTargetContext(prevMouseX, prevMouseY);
        //Which context is under the current mouse
        ContextInfo* curMouseInfo = FindTargetContext(curMouseX, curMouseY);
        
        if(prevMouseInfo != 0)
            IPCSend(prevMouseInfo->clientID, IPC_TYPE_GUI_EVENT, EVENT_TYPE_MOUSEMOVE, prevMouseX, prevMouseY, curMouseX, curMouseY);
        
        if(curMouseInfo != 0 && curMouseInfo != prevMouseInfo)
            IPCSend(curMouseInfo->clientID, IPC_TYPE_GUI_EVENT, EVENT_TYPE_MOUSEMOVE, prevMouseX, prevMouseY, curMouseX, curMouseY);
    
    }

    prevMouseLeft = mouseLeft;
    prevMouseMiddle = mouseMiddle;
    prevMouseRight = mouseRight;


    // Process all the pressed keys
    while(Process::StdInAvailable() > 0)
    {
        char key = Process::ReadStdIn();
        ContextInfo* sendTo = contextList->GetAt(0); //Send key to the context currenly in focus
        IPCSend(sendTo->clientID, IPC_TYPE_GUI_EVENT, EVENT_TYPE_KEYPRESS, (uint32_t)key, (uint32_t)sendTo->id);
    }
}