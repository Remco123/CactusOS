#include <system/memory/deviceheap.h>
#include <system/memory/heap.h>
#include <system/log.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

uint32_t DeviceHeap::currentAddress = KERNEL_HEAP_START + KERNEL_HEAP_SIZE + 1_MB; // 1MB Padding

uint32_t DeviceHeap::AllocateChunck(uint32_t size)
{
    uint32_t ret = DeviceHeap::currentAddress;
    DeviceHeap::currentAddress += size;

    // Will propably never happen
    if(DeviceHeap::currentAddress >= PAGE_TABLE_ADDRESS)
        Log(Error, "Device heap is about to flow into page tables!");
    
    return ret;
}