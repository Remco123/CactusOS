#include <core/pit.h>

using namespace CactusOS::common;
using namespace CactusOS::core;

void printf(char*);

PIT::PIT(InterruptManager* interrupts)
: InterruptHandler(interrupts, interrupts->HardwareInterruptOffset() + 0)
{
    timer_ticks = 0;

    uint64_t divisor = 1193180 / 1000; //Default is 1000 Hz

    outportb(0x43, 0x36);
    outportb(0x40, (uint8_t)divisor);
    outportb(0x40, (uint8_t)(divisor >> 8));
}

PIT::~PIT()
{
    
}

uint32_t PIT::HandleInterrupt(uint32_t esp)
{
    timer_ticks++;

    return esp;
}
void PIT::Sleep(uint32_t ms)
{
    //MS is the same as ticks because freq = 1000
    unsigned long eticks;

    eticks = timer_ticks + ms;
    while(timer_ticks < eticks);
}


void PIT::PlaySound(common::uint32_t nFrequence)
{
    uint32_t Div;
 	uint8_t tmp;
 
    //Set the PIT to the desired frequency
 	Div = 1193180 / nFrequence;
 	outportb(0x43, 0xb6);
 	outportb(0x42, (uint8_t) (Div) );
 	outportb(0x42, (uint8_t) (Div >> 8));
 
    //And play the sound using the PC speaker
 	tmp = inportb(0x61);
  	if (tmp != (tmp | 3)) {
 		outportb(0x61, tmp | 3);
 	}
}
void PIT::NoSound()
{
    uint8_t tmp = inportb(0x61) & 0xFC;
 
 	outportb(0x61, tmp);
}
void PIT::Beep()
{
    Beep(800); //800 is default beep frequency
}

void PIT::Beep(common::uint32_t freq)
{
    Beep(freq, 200); //200 is default beep duration
}
void PIT::Beep(common::uint32_t freq, common::uint32_t duration)
{
    if(duration == 0)
        return;
    if(freq == 0)
        return;

    PlaySound(freq);
 	Sleep(duration);
 	NoSound();
}