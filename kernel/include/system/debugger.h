#ifndef __CACTUSOS__SYSTEM__DEBUGGER_H
#define __CACTUSOS__SYSTEM__DEBUGGER_H

#include <core/registers.h>
#include <common/types.h>
#include <common/list.h>

namespace CactusOS
{
    namespace system
    {
        // Entry in a kernel symbol dump
        typedef struct
        {
            char*               name;
            common::uint32_t    address;
            char                type;
        } KernelSymbol_t;

        // Entry in a stacktrace
        typedef struct StackFrame {
            struct StackFrame*  next;
            common::uint32_t    addr;
        } __attribute__((packed)) StackFrame_t;


        // A class that handles debugging of the kernel
        // Stuff like monitoring and providing stacktraces
        class KernelDebugger
        {
        private:
            // List of all known kernel symbols
            static List<KernelSymbol_t> symbolTable;

            // Buffer to store debug commands
            static char messageBuffer[200];

            // Address which we can map to physical regions
            static uint32_t pageAccessAddress;

            static const char* FindSymbol(uint32_t address, uint32_t* offset);

            static void HandleDebugCommand(int size);
        public:
            // Initialize debugger by loading symbol file from disk
            static void Initialize();

            // Print a stacktrace to console of given cpu state
            static void Stacktrace(core::CPUState* esp);
            
            // Perform a update on statistics and send info to debugger via serial
            static void Update();

            // Send a update on kernel stats to debugging host
            static void SendUpdateToHost();

            // Print a memory dump to the console
            static void PrintMemoryDump(uint32_t address, uint32_t size, bool virtMemory);

            // Print all page tables to the console
            static void PrintPageTables();
        };
    }
}

#endif