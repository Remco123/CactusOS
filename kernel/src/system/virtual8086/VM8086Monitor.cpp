#include <system/virtual8086/VM86Monitor.h>
#include <common/memoryoperations.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::core;

extern CPUState vm86CPUState;

Virtual8086Monitor::Virtual8086Monitor()
: InterruptHandler(0xD)
{ }

static uint8_t peekb(uint16_t seg, uint16_t offs)
{
    return *(uint8_t *)((seg << 4) + offs);
}

static uint16_t peekw(uint16_t seg, uint16_t offs)
{
    return *(uint16_t *)((seg << 4) + offs);
}

static void pokeb(uint16_t seg, uint16_t offs, uint8_t val)
{
    *(uint8_t *)((seg << 4) + offs) = val;
}

static void pokew(uint16_t seg, uint16_t offs, uint16_t val)
{
    *(uint16_t *)((seg << 4) + offs) = val;
}

static void pushw(CPUState* state, uint16_t val)
{
    state->UserSP -= 2;
    pokew(state->UserSS, state->UserSP, val);
}

static uint16_t popw(CPUState* state)
{
    uint16_t res = peekw(state->UserSS, state->UserSP);
    state->UserSP += 2;
    return res;
}

uint32_t Virtual8086Monitor::HandleInterrupt(uint32_t esp)
{
    CPUState* state = (CPUState*)esp;

    uint8_t opcode = peekb(state->CS, state->IP);
    if(opcode == 0xEE)
    {
        outportb(state->DX, state->AL);
        ++state->IP;
        return esp;
    }
    else if(opcode == 0xEF)
    {
        uint8_t prefix = peekb(state->CS, state->IP - 1);
        if(prefix == 0x66)
            outportl(state->DX, state->EAX);
        else
            outportw(state->DX, state->AX);
        ++state->IP;
        return esp;
    }
    else if(opcode == 0xEC)
    {
        state->AL = inportb(state->DX);
        ++state->IP;
        return esp;
    }
    else if(opcode == 0xED)
    {
        uint8_t prefix = peekb(state->CS, state->IP - 1);
        if(prefix == 0x66)
            state->EAX = inportl(state->DX);
        else
            state->AX = inportw(state->DX);
        ++state->IP;
        return esp;
    }
    else if(opcode == 0xE6)
    {
        uint8_t port = peekb(state->CS, ++state->IP);
        outportb(port, state->AL);
        ++state->IP;
        return esp;
    }
    else if(opcode == 0xE7)
    {
        uint8_t prefix = peekb(state->CS, state->IP - 1);
        uint8_t port = peekb(state->CS, ++state->IP);
        if(prefix == 0x66)
            outportl(port, state->EAX);
        else
            outportw(port, state->AX);
        ++state->IP;
        return esp;
    }
    else if(opcode == 0xE4)
    {
        uint8_t port = peekb(state->CS, ++state->IP);
        state->AL = inportb(port);
        ++state->IP;
        return esp;
    }
    else if(opcode == 0xE5)
    {
        uint8_t prefix = peekb(state->CS, state->IP - 1);
        uint8_t port = peekb(state->CS, ++state->IP);
        if(prefix == 0x66)
            state->EAX = inportl(port);
        else
            state->AX = inportw(port);
        ++state->IP;
        return esp;
    }
    else if(opcode == 0xCD)
    { // int n
        uint8_t intNo = peekb(state->CS, ++state->IP);
        if(intNo == 0xFE)
        { // special interrupt
            MemoryOperations::memcpy(state, &vm86CPUState, sizeof(CPUState));
            return esp;
        }
        pushw(state, state->FLAGS);
        state->EFLAGS &= ~((1 << 8) | (1 << 9) | (1 << 18));
        pushw(state, state->CS);
        pushw(state, ++state->IP);
        state->CS = peekw(0x0000, 4 * intNo + 2);
        state->IP = peekw(0x0000, 4 * intNo);
        return esp;
    }
    else if(opcode == 0xCF)
    { // iret
        state->IP = popw(state);
        state->CS = popw(state);
        state->FLAGS = popw(state);
        return esp;
    }
    else if(opcode == 0x9C)
    { // pushf
        pushw(state, state->FLAGS);
        ++state->IP;
        return esp;
    }
    else if(opcode == 0x9D)
    { // popf
        state->FLAGS = popw(state);
        ++state->IP;
        return esp;;
    }
    else if(opcode == 0xFA)
    { // cli
        state->FLAGS &= ~(1 << 9);
        ++state->IP;
        return esp;
    }
    else if(opcode == 0xFB)
    { // sti
        state->FLAGS |= 1 << 9;
        ++state->IP;
        return esp;
    }

    BootConsole::WriteLine("[vm86] Unknown instruction");
    return esp;
}
