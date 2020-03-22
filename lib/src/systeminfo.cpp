#include <systeminfo.h>
#include <syscall.h>
#include <proc.h>
#include <log.h>

using namespace LIBCactusOS;

bool LIBCactusOS::RequestSystemInfo()
{
    bool ret = DoSyscall(SYSCALL_MAP_SYSINFO, SYSTEM_INFO_ADDR);
    if(ret)
        Process::systemInfo = (SharedSystemInfo*)SYSTEM_INFO_ADDR;
    
    return ret;
}