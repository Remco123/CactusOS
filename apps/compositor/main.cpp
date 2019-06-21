#include <new.h>
#include <log.h>
#include <gui/directgui.h>
#include <ipc.h>
#include <syscall.h>
#include <gui/canvas.h>
#include <gui/gui.h>
#include <list.h>
#include <time.h>
#include <proc.h>
#include <string.h>
#include <systeminfo.h>
#include "contextinfo.h"
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
List<ContextInfo>* contextList;
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

    contextList = new List<ContextInfo>();
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

        Print("Got Request from %d\n", msg.source);
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
        case COMPOSITOR_REQUESTCONTEXT:
        {
            uint32_t width = msg.arg3;
            uint32_t height = msg.arg4;
            uint32_t x = msg.arg5;
            uint32_t y = msg.arg6;

            uint32_t bytes = width * height * 4;
            uint32_t virtAddrC = msg.arg2;
            Print("Process %d requested a gui context of %d bytes at %x (w=%d,h=%d,x=%d,y=%d)\n", msg.source, bytes, virtAddrC, width, height, x, y);
            if(Process::CreateSharedMemory(msg.source, newContextAddress, virtAddrC, pageRoundUp(bytes)) == false) {
                Print("Error creating shared memory\n");
                break;
            }

            ContextInfo info;
            info.bytes = bytes;
            info.virtAddrClient = virtAddrC;
            info.virtAddrServer = newContextAddress;
            info.width = width;
            info.height = height;
            info.x = x;
            info.y = y;
            info.clientID = msg.source;

            newContextAddress += pageRoundUp(bytes);
            contextList->push_back(info);

            //Send response to client
            IPCSend(msg.source, IPC_TYPE_GUI, 1);
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

    for(ContextInfo info : *contextList)
    {
        if(info.x >= WIDTH || info.y >= HEIGHT) {
            Log(Warning, "Context is out of desktop bounds");
            continue;
        }
        
        uint32_t byteWidth = (info.width + info.x <= WIDTH ? info.width : info.width-(info.x + info.width - WIDTH))*4;
        for(uint32_t y = 0; y < info.height; y++)
            memcpy((void*)(backBuffer + ((info.y + y)*WIDTH*4) + info.x*4), (void*)(info.virtAddrServer + y*info.width*4), byteWidth);
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

    if(mouseLeft!=prevMouseLeft || mouseRight!=prevMouseRight || mouseMiddle!=prevMouseMiddle)
    {
        for(ContextInfo info : *contextList)
        {
            if(curMouseX >= info.x && curMouseX <= info.x + info.width)
                if(curMouseY >= info.y && curMouseY <= info.y + info.height)
                {
                    //Log(Info, "Mouse click inside context, sending message.");
                    uint8_t changedButton;
                    if(mouseLeft!=prevMouseLeft)
                        changedButton = 0;
                    else if(mouseMiddle!=prevMouseMiddle)
                        changedButton = 1;
                    else
                        changedButton = 2;

                    //Check if the mouse has been held down or up
                    bool mouseDown = changedButton == 0 ? mouseLeft : (changedButton == 1 ? mouseMiddle : (changedButton == 2 ? mouseRight : 0));
                    IPCSend(info.clientID, IPC_TYPE_GUI_EVENT, mouseDown ? EVENT_TYPE_MOUSEDOWN : EVENT_TYPE_MOUSEUP, curMouseX, curMouseY, changedButton);
                    break;
                }
        }
    }

    prevMouseLeft = mouseLeft;
    prevMouseMiddle = mouseMiddle;
    prevMouseRight = mouseRight;
}