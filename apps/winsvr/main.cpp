#include <new.h>
#include <log.h>
#include <gui/directgui.h>
#include <ipc.h>
#include <syscall.h>
#include <gui/guicom.h>
#include <list.h>
#include <time.h>
#include <proc.h>
#include <string.h>
#include <systeminfo.h>
#include "contextinfo.h"
#include "cursor.h"

using namespace LIBCactusOS;

void HandleMessage(IPCMessage msg);
void DrawContextList();
void DrawCursor();

List<ContextInfo>* contextList;
uint32_t newContextAddress = 0xA0000000;

void DrawLoop()
{
    Print("Draw loop started\n");
    while(true)
        DrawContextList();
}

int main()
{
    Print("Starting Window Manager\n");
    if(!DirectGUI::RequestFramebuffer()) {
        Log(Error, "Error initializing framebuffer");
        return -1;
    }
    Print("Framebuffer Initialized\n");

    Print("Requesting Systeminfo\n");
    if(!RequestSystemInfo())
        return -1;

    contextList = new List<ContextInfo>();
    Print("Listening for requests\n");
    
    bool receivedMessage = false;
    while (1)
    {
        int msgError = 0;
        IPCMessage msg = ICPReceive(-1, &msgError);

        if(msgError == SYSCALL_RET_ERROR || msg.type != IPC_TYPE_GUI) {
            Print("Something wrong with message, ignoring\n");
            continue;
        }

        Print("Got Request from %d\n", msg.source);
        HandleMessage(msg);

        if(!receivedMessage) //First time we receive something start the draw thread
        {
            receivedMessage = true;
            Print("Creating draw thread\n");
            Process::CreateThread(DrawLoop, false);
        }
    }

    return 0;
}

void HandleMessage(IPCMessage msg)
{
    int msgType = msg.arg1;
    switch (msgType)
    {
        case GUICOM_REQUESTCONTEXT:
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

void DrawContextList()
{    
    for(ContextInfo info : *contextList)
    {
        uint32_t byteWidth = info.width*4;
        for(uint32_t y = 0; y < info.height; y++)
        {
            memcpy((void*)(DIRECT_GUI_ADDR + ((info.y + y)*WIDTH*4) + info.x*4), (void*)(info.virtAddrServer + y*byteWidth), byteWidth);
        }
    }
    DrawCursor();
}

void DrawCursor()
{
    uint8_t x_d = Process::systemInfo->MouseX + 12 < WIDTH ? 12 : WIDTH - Process::systemInfo->MouseX;
    uint8_t y_d = Process::systemInfo->MouseY + 19 < HEIGHT ? 19 : HEIGHT - Process::systemInfo->MouseY;

    for(uint8_t x = 0; x < x_d; x++)
        for(uint8_t y = 0; y < y_d; y++) {
            uint8_t cd = cursorBitmap[y*12+x];
            if(cd != ALPHA)
                DirectGUI::SetPixel(Process::systemInfo->MouseX + x, Process::systemInfo->MouseY + y, cd==WHITE ? 0xFFFFFFFF : 0xFF000000);
        }
}