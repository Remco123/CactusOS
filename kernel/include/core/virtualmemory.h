/*
*   Structs from: https://wiki.osdev.org/Paging
*/

#ifndef __CACTUSOS__CORE__VIRTUALMEMORY_H
#define __CACTUSOS__CORE__VIRTUALMEMORY_H

#include <core/physicalmemory.h>
#include <common/types.h>
#include <system/memory/heap.h>

namespace CactusOS
{
    namespace core
    {
        extern "C" void* BootPageDirectory;

        enum PAGE_ENTRY_SIZE
        {
            FOUR_KB = 0,
            FOUR_MB = 1
        };

        #define KERNEL_VIRT_ADDR 3_GB
        #define USER_STACK_SIZE 32_KB
        #define USER_STACK_TOP (KERNEL_VIRT_ADDR)
        #define USER_STACK (USER_STACK_TOP - USER_STACK_SIZE)

        #define PAGE_SIZE 4_KB
        #define KERNEL_PTNUM 768 //The kernel is in the 768th entry
        #define PAGE_TABLE_ADDRESS 0xFFC00000
        #define PAGE_DIRECTORY_ADDRESS 0xFFFFF000

        #define PAGE_OFFSET_BITS 12

        #define PAGEDIR_INDEX(addr) (((uint32_t)addr) >> 22)
        #define PAGETBL_INDEX(addr) ((((uint32_t)addr) >> 12) & 0x3ff)
        #define PAGEFRAME_INDEX(addr) (((uint32_t)addr) & 0xfff)

        struct PageDirectoryEntry
        {
            common::uint32_t present        : 1;    //Is this page present in physical memory or perhaps on the disk.
            common::uint32_t readWrite      : 1;    //If set the page is r/w otherwise read only.
            common::uint32_t isUser         : 1;    //If set the page can be accessed by all levels.
            common::uint32_t writeThrough   : 1;    //If set write through caching is enabled.
            common::uint32_t canCache       : 1;    //If the bit is set, the page will not be cached.
            common::uint32_t accessed       : 1;    //Has this page been accesed, set by cpu.
            common::uint32_t reserved       : 1;    //Reserved.
            common::uint32_t pageSize       : 1;    //If the bit is set, then pages are 4 MiB in size. Otherwise, they are 4 KiB.
            common::uint32_t ignored        : 1;
            common::uint32_t unused         : 3;    //Unused by the cpu so we can use them perhaps for additional page information.
            common::uint32_t frame          : 20;   //Page Table 4-kb aligned address.
        } __attribute__((packed));

        struct PageTableEntry
        {
            common::uint32_t present        : 1;    //Is this page present in physical memory or perhaps on the disk.
            common::uint32_t readWrite      : 1;    //If set the page is r/w otherwise read only.
            common::uint32_t isUser         : 1;    //If set the page can be accessed by all levels.
            common::uint32_t writeThrough   : 1;    //If set write through caching is enabled.
            common::uint32_t canCache       : 1;    //If the bit is set, the page will not be cached.
            common::uint32_t accessed       : 1;    //Has this page been accesed, set by cpu.
            common::uint32_t dirty          : 1;    //Indicates that the page has been written to.
            common::uint32_t reserved       : 1;
            common::uint32_t global         : 1;    //if set, prevents the TLB from updating the address in its cache if CR3 is reset. Note, that the page global enable bit in CR4 must be set to enable this feature. 
            common::uint32_t unused         : 3;    //Unused by the cpu so we can use them perhaps for additional page information.
            common::uint32_t frame          : 20;   //Target memory 4kb block
        } __attribute__((packed));

        struct PageTable
        {
            PageTableEntry entries[1024];
        }   __attribute__((packed));

        struct PageDirectory
        {
            PageDirectoryEntry entries[1024];
        }   __attribute__((packed));

        // Invalidate TLB Entries
        static inline void invlpg(void* addr)
        {
            asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
        }

        class VirtualMemoryManager
        {
        public:      
            static void ReloadCR3();  
            static void Initialize();
            static void AllocatePage(PageTableEntry* page, bool kernel, bool writeable);
            static void FreePage(PageTableEntry* page);

            static PageTableEntry* GetPageForAddress(common::uint32_t virtualAddress, bool shouldCreate, bool readWrite = true, bool userPages = false);
            
            static void* GetPageTableAddress(common::uint16_t pageTableNumber);
            
            static void* virtualToPhysical(void* virtAddress);
            
            static void mapVirtualToPhysical(void* physAddress, void* virtAddress, bool kernel = true, bool writeable = true);
            static void mapVirtualToPhysical(void* physAddress, void* virtAddress, common::uint32_t size, bool kernel = true, bool writeable = true);
            
            static void SwitchPageDirectory(common::uint32_t physAddr);
            static common::uint32_t GetPageDirectoryAddress();
        };
    }
}

#endif