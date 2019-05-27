#include <systeminfo.h>
#include <syscall.h>
#include <proc.h>
#include <log.h>

using namespace LIBCactusOS;

bool LIBCactusOS::RequestSystemInfo(unsigned int mapTo)
{
    bool ret = DoSyscall(SYSCALL_MAP_SYSINFO, mapTo);
    if(ret)
        Process::systemInfo = (SharedSystemInfo*)mapTo;
    
    return ret;
}