#include <gui/guicom.h>
#include <ipc.h>
#include <syscall.h>
#include <heap.h>

using namespace LIBCactusOS;

int GUICommunication::windowServerID = 3;
uint32_t GUICommunication::virtualFramebufferAddress = 0xA0000000;

uint32_t GUICommunication::RequestContext(int width, int height, int x, int y)
{
    if(IPCSend(windowServerID, IPC_TYPE_GUI, GUICOM_REQUESTCONTEXT, virtualFramebufferAddress, width, height, x, y) != SYSCALL_RET_SUCCES)
        return 0;

    //Wait for response from server
    IPCMessage response = ICPReceive(windowServerID);
    if(response.type == IPC_TYPE_GUI && response.arg1 == 1) {
        uint32_t oldFB = virtualFramebufferAddress;
        uint32_t newFB = pageRoundUp(oldFB + width*height*4);
        
        virtualFramebufferAddress = newFB;
        return oldFB;
    }
    
    return 0;
}