/*
http://www.petesqbsite.com/sections/tutorials/tuts/vbe3.pdf
*/

#include <system/components/vesa.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::core;

uint16_t* RealPTRToAddr(Real_Pointer ptr){
    return (uint16_t*)((ptr[1] * 0x10) + ptr[0]);
}

VESA::VESA(Virtual8086Manager* vm86)
: SystemComponent("VESA VBE", "VESA BIOS Extensions")
{ 
    this->virtual8086Manager = vm86;
}

VESAModeInfo* VESA::GetModeInfo(uint16_t mode)
{
    VESAModeInfo* info = (VESAModeInfo*)0x8000;
    MemoryOperations::memset(info, 0, sizeof(VESAModeInfo));

	VM86Registers regs;
	MemoryOperations::memset(&regs, 0, sizeof(VM86Registers));

	regs.AX = 0x4F01;
	regs.CX = mode;
	regs.ES = 0;
	regs.DI = (uint16_t)info;
	virtual8086Manager->CallInterrupt(0x10, &regs);	
	
	return info;
}

void VESA::SetVideoMode(uint16_t mode)
{
	VM86Registers regs;
	MemoryOperations::memset(&regs, 0, sizeof(VM86Registers));
	regs.AX = 0x4f02;
	regs.BX = mode;
	virtual8086Manager->CallInterrupt(0x10, &regs);
}

bool VESA::SelectBestVideoMode()
{
	BootConsole::WriteLine("Selecting Best Video Mode");
	VESAControllerInfo* vesaInfo = (VESAControllerInfo*)0x7000;
	MemoryOperations::memset(vesaInfo, 0, 512);
	MemoryOperations::memcpy(vesaInfo->Signature, "VBE2", 4);
	
	BootConsole::WriteLine("Probing for VBE controller Information");
	VM86Registers regs;
	MemoryOperations::memset(&regs, 0, sizeof(VM86Registers));
	regs.AX = 0x4f00;
	regs.DI = (uint16_t)vesaInfo;
	virtual8086Manager->CallInterrupt(0x10, &regs);

	if(MemoryOperations::memcmp(vesaInfo->Signature, "VESA", 4) == 0)
	{
		BootConsole::Write("VBE Version: 0x"); Print::printfHex16(vesaInfo->Version); BootConsole::WriteLine();
		BootConsole::Write("OEM String: "); BootConsole::WriteLine((char*)RealPTRToAddr(vesaInfo->OemStringPtr));
		BootConsole::Write("Video Memory: "); BootConsole::Write(Convert::IntToString(vesaInfo->TotalMemory * 64)); BootConsole::WriteLine(" Kb"); 

		uint16_t* modeArray = (uint16_t*)RealPTRToAddr(vesaInfo->VideoModePtr);
		BootConsole::WriteLine("Looping through video modes");

		uint16_t selectedModeNumber = 0;

		while(*modeArray != 0xFFFF) {
			VESAModeInfo* modeInfo = GetModeInfo(*modeArray);

			BootConsole::Write("0x"); Print::printfHex16(*modeArray);
			BootConsole::Write(" X: "); BootConsole::Write(Convert::IntToString(modeInfo->XResolution));
			BootConsole::SetX(18);
			BootConsole::Write(" Y: "); BootConsole::Write(Convert::IntToString(modeInfo->YResolution));
			BootConsole::SetX(30);
			BootConsole::Write(" BPP: "); BootConsole::WriteLine(Convert::IntToString(modeInfo->BitsPerPixel));

			//TODO: Add algoritm for this or ask the user
			if(modeInfo->XResolution == 1024 && modeInfo->YResolution == 768 && modeInfo->BitsPerPixel == 32)
				selectedModeNumber = *modeArray;

			modeArray++;
		}

		if(selectedModeNumber != 0)
		{
			BootConsole::Write("Switching to video mode: 0x"); Print::printfHex16(selectedModeNumber); BootConsole::WriteLine();
			this->SetVideoMode(selectedModeNumber);

			//Set the current mode info
			MemoryOperations::memcpy(&this->currentVideoMode, GetModeInfo(selectedModeNumber), sizeof(VESAModeInfo));

			BootConsole::Write("Framebuffer is at: 0x"); Print::printfHex32(this->currentVideoMode.PhysBasePtr); BootConsole::WriteLine();

			BootConsole::WriteLine("Mapping framebuffer");
			VirtualMemoryManager::mapVirtualToPhysical((void*)this->currentVideoMode.PhysBasePtr, (void*)0xE0000000, this->currentVideoMode.YResolution * this->currentVideoMode.BytesPerScanLine, true, true);

			return true;
		}
		else
		{
			BootConsole::WriteLine("Could not find a usable video mode");
			return false;
		}
	}
	else
	{
		BootConsole::WriteLine("Vesa info block does not have valid signature");
		return false;
	}
}

uint32_t VESA::GetBufferSize()
{
	return this->currentVideoMode.XResolution * this->currentVideoMode.YResolution * (this->currentVideoMode.BitsPerPixel/8);
}