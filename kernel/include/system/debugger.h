#ifndef __CACTUSOS__SYSTEM__DEBUGGER_H
#define __CACTUSOS__SYSTEM__DEBUGGER_H

#include <core/registers.h>
#include <common/types.h>
#include <common/list.h>

namespace CactusOS
{
    namespace system
    {
        // Entry in a symbol dump
        typedef struct
        {
            char*               name;
            common::uint32_t    address;
            char                type;
        } GenericSymbol_t;

        // Entry in a stacktrace
        typedef struct StackFrame {
            struct StackFrame*  next;
            common::uint32_t    addr;
        } __attribute__((packed)) StackFrame_t;


        // A debugging class that works of a file with symbol definitions
        // Keeping this a class makes it possible to use this for userspace processes as well (if the symbol file is generated of course)
        class SymbolDebugger
        {
        private:
            // List of all known symbols
            List<GenericSymbol_t> symbolTable;

            // Buffer to store debug commands
            char messageBuffer[200] = {0};

            // Address which we can map to physical regions
            uint32_t pageAccessAddress = 0;

            bool isKernel = false;
            int serialIndex = 0;

            const char* FindSymbol(uint32_t address, uint32_t* offset);

            void HandleDebugCommand(int size);
            void PrintPageItem(void* item, bool table, uint16_t pdIndex, uint16_t ptIndex);
        public:
            // Initialize debugger by loading symbol file from disk
            SymbolDebugger(char* symFile, bool kernel = false);

            // Print a stacktrace to console of given cpu state
            void Stacktrace(core::CPUState* esp);
            
            // Perform a update on statistics and send info to debugger via serial
            void Update();

            // Send a update on stats to debugging host
            void SendUpdateToHost();

            // Print a memory dump to the console
            void PrintMemoryDump(uint32_t address, uint32_t size, bool virtMemory);

            // Print all page tables to the console
            void PrintPageTables(int pid = -1);
        };
    }
}

#endif