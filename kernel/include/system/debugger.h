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

            static const char* FindSymbol(uint32_t address, uint32_t* offset);
        public:
            // Initialize debugger by loading symbol file from disk
            static void Initialize();

            // Print a stacktrace to console of given cpu state
            static void Stacktrace(core::CPUState* esp);
        };
    }
}

#endif