#ifndef __CACTUSOS__CORE__PHYSICALMEMORY_H
#define __CACTUSOS__CORE__PHYSICALMEMORY_H

#include <common/types.h>
#include <common/convert.h>
#include <common/memoryoperations.h>
#include <system/bootconsole.h>
#include <multiboot/multiboot.h>

namespace CactusOS
{
    namespace core
    {
        #define BLOCK_SIZE 4_KB
        #define BLOCKS_PER_BYTE 8 //Every byte describes 8 blocks

        typedef struct multiboot_memory_map {
            unsigned int size;
            unsigned long base_addr_low;
            unsigned long base_addr_high;
            unsigned long length_low;
            unsigned long length_high;
            unsigned int type;
        }  __attribute__((packed)) grub_multiboot_memory_map_t;

        class PhysicalMemoryManager
        {
        private:
            static common::uint32_t memorySize;
            static common::uint32_t usedBlocks;
            static common::uint32_t maximumBlocks;
            static common::uint32_t* memoryArray;

            static inline void SetBit (common::uint32_t bit)
            {
                memoryArray[bit / 32] |= (1 << (bit % 32));
            }
            static inline void UnsetBit (common::uint32_t bit)
            {
                memoryArray[bit / 32] &= ~ (1 << (bit % 32));
            }
            static inline bool TestBit (common::uint32_t bit)
            {
                return memoryArray[bit / 32] &  (1 << (bit % 32));
            }

            static common::uint32_t FirstFree ();
            static common::uint32_t FirstFreeSize (common::uint32_t size);
        public:
            static void Initialize(common::uint32_t size, common::uint32_t bitmap);
            static void SetRegionFree(common::uint32_t base, common::uint32_t size);
            static void SetRegionUsed(common::uint32_t base, common::uint32_t size);
            static void ParseMemoryMap(const multiboot_info_t* mbi);
            
            static void* AllocateBlock();
            static void FreeBlock(void* ptr);
            static void* AllocateBlocks(common::uint32_t size);
            static void FreeBlocks(void* ptr, common::uint32_t size);

            static common::uint32_t AmountOfMemory();
            static common::uint32_t UsedBlocks();
            static common::uint32_t FreeBlocks();
            static common::uint32_t TotalBlocks();
            static common::uint32_t GetBitmapSize();
        };

        //Helper functions
        common::uint32_t pageRoundUp(common::uint32_t address);
        common::uint32_t pageRoundDown(common::uint32_t address);
    }
}

#endif