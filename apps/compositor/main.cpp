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
#include <math.h>
#include <gui/contextheap.h>
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
void ResizeContext(ContextInfo* c, Rectangle newSize);
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
/**
 * Maximum distance between cursor and edge to see it as resizing
*/
const int resizeMaxDistance = 4;
/**
 * Color of border for resizing
*/
const uint32_t resizeBorderColor = 0xFFAAAAAA;
/**
 * Color of rect for resizing
*/
const uint32_t resizeRectColor = 0xFF00FF00;
/**
 * On which context is the mouse in resize borders
*/
ContextInfo* drawResizing = 0;
/**
 * In which border is the mouse of the drawResizing context
*/
Direction drawResizeDirection = None;
/**
 * Which context are we currently resizing
*/
ContextInfo* currentlyResizing = 0;
/**
 * In which direction are we currently resizing a context
*/
Direction currentResizeDirection = None;
/**
 * This rect will hold the new size of the context while the mouse is moving around
*/
Rectangle resizeRectangle(0, 0, 0, 0);

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

        ////////
        // Switch processes after drawing desktop
        // It is useless to draw it like 30 times in a couple milliseconds.
        Process::Yield();
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
// Makes rectangle fit into desktop rectangle
void ApplyDesktopBounds(Rectangle* rect)
{
    if(rect->x < 0) {
        rect->width -= Math::Abs(rect->x);
        rect->x = 0;
    }
    if(rect->y < 0) {
        rect->height -= Math::Abs(rect->y);
        rect->y = 0;
    }
    if((rect->x + rect->width) >= WIDTH) {
        rect->width = WIDTH - rect->x;// - 1;
    }
    if((rect->y + rect->height) >= HEIGHT) {
        rect->height = HEIGHT - rect->y;// - 1;
    }
}
// Check for the given context if the mouse is in a resize border
void CheckContextResizeBorders(int mouseX, int mouseY, ContextInfo* c, bool* top, bool* right, bool* bottom, bool* left)
{
    Rectangle cr(c->width, c->height, c->x, c->y);

    *top = Math::Abs(cr.y - mouseY) < resizeMaxDistance                 &&    (c->resizeDirections & Top);
    *right = Math::Abs(cr.x + cr.width - mouseX) < resizeMaxDistance    &&    (c->resizeDirections & Right);
    *bottom = Math::Abs(cr.y + cr.height - mouseY) < resizeMaxDistance  &&    (c->resizeDirections & Bottom);
    *left = Math::Abs(cr.x - mouseX) < resizeMaxDistance                &&    (c->resizeDirections & Left);
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

    Print("Preparing Context Memory allocation\n");
    ContextHeap::Init();

    currentlyResizing = 0;
    drawResizing = 0;
    currentResizeDirection = None;
    drawResizeDirection = None;

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

            uint32_t bytes = WIDTH * HEIGHT * 4 + sizeof(ContextInfo); //TODO: Actualy use width and height and realocate memory when resizing
            uint32_t virtAddrC = msg.arg2;
            uint32_t contextAddress = ContextHeap::AllocateArea(pageRoundUp(bytes) / 0x1000);
            Print("Process %d requested a gui context of %d bytes at %x (w=%d,h=%d,x=%d,y=%d) mapping to %x\n", msg.source, bytes, virtAddrC, width, height, x, y, contextAddress);
            if(Process::CreateSharedMemory(msg.source, contextAddress, virtAddrC, pageRoundUp(bytes)) == false) {
                Print("Error creating shared memory\n");
                break;
            }

            ContextInfo* info = (ContextInfo*)contextAddress;
            info->bytes = bytes;
            info->virtAddrClient = virtAddrC + sizeof(ContextInfo);
            info->virtAddrServer = contextAddress + sizeof(ContextInfo);
            info->width = width;
            info->height = height;
            info->x = x;
            info->y = y;
            info->clientID = msg.source;
            info->supportsTransparency = false;
            info->background = false;
            info->allowResize = false;
            info->resizeDirections = (Top | Right | Bottom | Left);
            info->id = nextContextID++;

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

                    //Add a dirty rect at the old position of the context
                    Rectangle dirtyRect(c->width, c->height, c->x, c->y);
                    dirtyRectList->push_back(dirtyRect);

                    //Free area of virtual allocated memory
                    ContextHeap::FreeArea(c->virtAddrServer + sizeof(ContextInfo), pageRoundUp(c->bytes) / 0x1000);
                    
                    //Free shared memory
                    if(!Process::DeleteSharedMemory(c->clientID, c->virtAddrServer - sizeof(ContextInfo), c->virtAddrClient - sizeof(ContextInfo), pageRoundUp(c->bytes)))
                        Log(Error, "Could not remove shared memory");
                }
            }
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
    
    //Update dirty rectangles
    while(dirtyRectList->size() > 0)
    {
        Rectangle rect = dirtyRectList->GetAt(0);
        ApplyDesktopBounds(&rect);
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
        Rectangle contextRectangle = Rectangle(info->width, info->height, info->x, info->y);
        ApplyDesktopBounds(&contextRectangle);
        
        #define leftOffset ((info->x < 0) ? -info->x : 0)
        #define topOffset  ((info->y < 0) ? -info->y : 0)

        if(info->supportsTransparency) {
            for(int y = 0; y < contextRectangle.height; y++)
                for(int x = 0; x < contextRectangle.width; x++)
                    backBufferCanvas->SetPixel(contextRectangle.x + x, contextRectangle.y + y, Colors::AlphaBlend(wallPaperCanvas->GetPixel(contextRectangle.x + x, contextRectangle.y + y), *(uint32_t*)(info->virtAddrServer + (topOffset+y)*info->width*4 + (leftOffset+x)*4)));
        }
        else {
            for(int hOffset = 0; hOffset < contextRectangle.height; hOffset++)
                memcpy((backBuffer + (contextRectangle.y+hOffset)*WIDTH*4 + contextRectangle.x*4), (void*)(info->virtAddrServer + leftOffset*4 + (topOffset + hOffset)*info->width*4), contextRectangle.width * 4);
        }

        if(info == drawResizing) { //Context has mouse in resize border
            contextRectangle.width -= 1;
            contextRectangle.height -= 1;

            if(drawResizeDirection & Top)
                backBufferCanvas->DrawLine(resizeBorderColor, contextRectangle.x, contextRectangle.y, contextRectangle.x + contextRectangle.width, contextRectangle.y);
            if(drawResizeDirection & Right)
                backBufferCanvas->DrawLine(resizeBorderColor, contextRectangle.x + contextRectangle.width, contextRectangle.y, contextRectangle.x + contextRectangle.width, contextRectangle.y + contextRectangle.height);
            if(drawResizeDirection & Bottom)
                backBufferCanvas->DrawLine(resizeBorderColor, contextRectangle.x, contextRectangle.y + contextRectangle.height, contextRectangle.x + contextRectangle.width, contextRectangle.y + contextRectangle.height);
            if(drawResizeDirection & Left)
                backBufferCanvas->DrawLine(resizeBorderColor, contextRectangle.x, contextRectangle.y, contextRectangle.x, contextRectangle.y + contextRectangle.height);
        }
    }
    if(currentlyResizing != 0) { //We are resizing a context at the moment
        backBufferCanvas->DrawRect(resizeRectColor, resizeRectangle.x, resizeRectangle.y, resizeRectangle.width-1, resizeRectangle.height-1);
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

        uint8_t changedButton;
        if(mouseLeft!=prevMouseLeft)
            changedButton = 0;
        else if(mouseMiddle!=prevMouseMiddle)
            changedButton = 1;
        else
            changedButton = 2;

        if(info != 0) {
            bool sendEvent = true;
            if(changedButton == 0 && info->allowResize && mouseLeft) { //Left mouse button has changed, check for resize
                bool top, right, bottom, left;
                CheckContextResizeBorders(curMouseX, curMouseY, info, &top, &right, &bottom, &left);
                if(top || right || bottom || left) { //Start resizing context
                    currentlyResizing = info;
                    currentResizeDirection = ((top ? Top : None) | (right ? Right : None) | (bottom ? Bottom : None) | (left ? Left : None));
                    sendEvent = false;
                    resizeRectangle = Rectangle(info->width, info->height, info->x, info->y);

                    Print("GUI: Start window resize %d in dir %b\n", info->id, (int)currentResizeDirection);
                }
            }
            else if(changedButton == 0 && currentlyResizing != 0) //Mouse release while resizing context
                sendEvent = false;

            if(sendEvent) {
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
        if(changedButton == 0 && currentlyResizing != 0 && !mouseLeft) { //Left mouse up
            Print("GUI: Stop window resize of context %d\n", currentlyResizing->id);
            ResizeContext(currentlyResizing, resizeRectangle);

            //Remove green border from screen
            //Could be more efficient but this works fine
            dirtyRectList->push_back(Rectangle(WIDTH, HEIGHT, 0, 0));
            currentResizeDirection = None;
            currentlyResizing = 0;
        }
    }

    ////////////
    // Mouse has moved
    ////////////
    if(curMouseX != prevMouseX || curMouseY != prevMouseY)
    {
        //Reset resize vars
        drawResizing = 0;
        drawResizeDirection = None;

        //Which context was under the previous mouse
        ContextInfo* prevMouseInfo = FindTargetContext(prevMouseX, prevMouseY);
        //Which context is under the current mouse
        ContextInfo* curMouseInfo = FindTargetContext(curMouseX, curMouseY);
        
        if(prevMouseInfo != 0)
            IPCSend(prevMouseInfo->clientID, IPC_TYPE_GUI_EVENT, EVENT_TYPE_MOUSEMOVE, prevMouseX, prevMouseY, curMouseX, curMouseY);
        
        if(curMouseInfo != 0 && curMouseInfo != prevMouseInfo)
            IPCSend(curMouseInfo->clientID, IPC_TYPE_GUI_EVENT, EVENT_TYPE_MOUSEMOVE, prevMouseX, prevMouseY, curMouseX, curMouseY);
        
        // Check for resize event
        if(curMouseInfo != 0 && curMouseInfo->allowResize && currentlyResizing == 0) {            
            bool top, right, bottom, left;
            CheckContextResizeBorders(curMouseX, curMouseY, curMouseInfo, &top, &right, &bottom, &left);

            if(top || right || bottom || left) {
                drawResizing = curMouseInfo;
                drawResizeDirection = ((top ? Top : None) | (right ? Right : None) | (bottom ? Bottom : None) | (left ? Left : None));
            }
        }
        else if(currentlyResizing != 0 && currentlyResizing->allowResize) {
            //Print("GUI: Window Resize of context %d in dir %b\n", currentlyResizing->id, (int)currentResizeDirection);

            Rectangle tempRect = resizeRectangle;
            if(currentResizeDirection & Top) {
                tempRect.y -= (prevMouseY - curMouseY);
                tempRect.height += (prevMouseY - curMouseY);
            }
            if(currentResizeDirection & Right) {
                tempRect.width -= (prevMouseX - curMouseX);
            }
            if(currentResizeDirection & Bottom) {
                tempRect.height -= (prevMouseY - curMouseY);
            }
            if(currentResizeDirection & Left) {
                tempRect.x -= (prevMouseX - curMouseX);
                tempRect.width += (prevMouseX - curMouseX);
            }
            ApplyDesktopBounds(&tempRect);
            //Add dirty rect for whole desktop, saves some calculations
            dirtyRectList->push_back(Rectangle(WIDTH, HEIGHT, 0, 0));
            resizeRectangle = tempRect;
        }
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
void ResizeContext(ContextInfo* c, Rectangle newSize)
{
    if(newSize.width < 0) {
        newSize.x += newSize.width;
        newSize.width = -newSize.width;
    }
    if(newSize.height < 0) {
        newSize.y += newSize.height;
        newSize.height = -newSize.height;
    }

    Rectangle oldSize = Rectangle(c->width, c->height, c->x, c->y);

    c->width = newSize.width;
    c->height = newSize.height;
    c->x = newSize.x;
    c->y = newSize.y;

    IPCSend(c->clientID, IPC_TYPE_GUI_EVENT, EVENT_TYPE_RESIZE, c->id, oldSize.width, oldSize.height, oldSize.x, oldSize.height);
}