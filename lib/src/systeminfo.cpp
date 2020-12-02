#include <systeminfo.h>
#include <syscall.h>
#include <proc.h>
#include <heap.h>
#include <string.h>
#include <types.h>
#include <log.h>

using namespace LIBCactusOS;

SIPropertyProvider SystemInfo::Properties = SIPropertyProvider("properties");

bool SystemInfo::RequestSystemInfo()
{
    bool ret = DoSyscall(SYSCALL_MAP_SYSINFO, SYSTEM_INFO_ADDR);
    if(ret)
        Process::systemInfo = (SharedSystemInfo*)SYSTEM_INFO_ADDR;
    
    return ret;
}



SIPropertyProvider::SIPropertyProvider(char* identifier)
{
    this->id = identifier;
    this->parent = 0;
}
SIPropertyProvider::SIPropertyProvider(int index)
{
    this->index = index;
    this->parent = 0;
}

int SIPropertyProvider::size()
{
    return this->receiveValue<int>(true);
}

SIPropertyProvider SIPropertyProvider::operator[](const int index)
{
    SIPropertyProvider ret = SIPropertyProvider(index);
    ret.parent = this;
    ret.type = Index;
    return ret;
}
SIPropertyProvider SIPropertyProvider::operator[](const char* element)
{
    SIPropertyProvider ret = SIPropertyProvider((char*)element);
    ret.parent = this;
    ret.type = String;
    return ret;
}


template<typename T>
T SIPropertyProvider::receiveValue(const bool getSize) const
{
    int count = 0;
    SIPropertyProvider* item = (SIPropertyProvider*)this;
    while(item) {
        count++;
        item = item->parent;
    }

    SIPropertyProvider* pathArray = (SIPropertyProvider*)UserHeap::Malloc(sizeof(SIPropertyProvider) * count);
    memset(pathArray, 0, sizeof(SIPropertyProvider) * count);

    item = (SIPropertyProvider*)this;
    for(int i = (count-1); i >= 0; i--) {
        pathArray[i] = *item;
        item = item->parent;
    }

    T returnValue = (T)0;
    if(DoSyscall(SYSCALL_GET_SYSINFO_VALUE, (uint32_t)pathArray, (uint32_t)&returnValue, count, (uint32_t)getSize) != SYSCALL_RET_SUCCES)
        Log(Error, "Could not get system property");

    UserHeap::Free(pathArray);
    return returnValue;
}

template<typename T>
T SIPropertyProvider::receiveBuffer() const
{
    int count = 0;
    SIPropertyProvider* item = (SIPropertyProvider*)this;
    while(item) {
        count++;
        item = item->parent;
    }

    SIPropertyProvider* pathArray = (SIPropertyProvider*)UserHeap::Malloc(sizeof(SIPropertyProvider) * count);
    memset(pathArray, 0, sizeof(SIPropertyProvider) * count);

    item = (SIPropertyProvider*)this;
    for(int i = (count-1); i >= 0; i--) {
        pathArray[i] = *item;
        item = item->parent;
    }

    T returnValue = (T)(new uint8_t[1_KB]);
    memset(returnValue, 0, 1_KB);
    if(DoSyscall(SYSCALL_GET_SYSINFO_VALUE, (uint32_t)pathArray, (uint32_t)returnValue, count) != SYSCALL_RET_SUCCES)
        Log(Error, "Could not get system property");

    UserHeap::Free(pathArray);
    return returnValue;
}

SIPropertyProvider::operator int() const
{    
    return this->receiveValue<int>(false);
}
SIPropertyProvider::operator bool() const
{
    return this->receiveValue<bool>(false);
}
SIPropertyProvider::operator double() const
{
    return this->receiveValue<double>(false);
}
SIPropertyProvider::operator char*() const
{
    return this->receiveBuffer<char*>();
}
SIPropertyProvider::operator uint64_t() const
{
    return this->receiveValue<uint64_t>(false);
}
SIPropertyProvider::operator uint32_t() const
{
    return this->receiveValue<uint32_t>(false);
}
SIPropertyProvider::operator uint16_t() const
{
    return this->receiveValue<uint16_t>(false);
}
SIPropertyProvider::operator uint8_t() const
{
    return this->receiveValue<uint8_t>(false);
}