#include <gui/guicom.h>
#include <ipc.h>
#include <syscall.h>

using namespace LIBCactusOS;

int GUICommunication::windowServerID = 3;

bool GUICommunication::RequestContext(uint32_t virtAddr, int width, int height, int x, int y)
{
    if(IPCSend(windowServerID, IPC_TYPE_GUI, GUICOM_REQUESTCONTEXT, virtAddr, width, height, x, y) != SYSCALL_RET_SUCCES)
        return false;

    //Wait for response from server
    IPCMessage response = ICPReceive(windowServerID);
    if(response.type == IPC_TYPE_GUI && response.arg1 == 1)
        return true;
    
    return false;
}