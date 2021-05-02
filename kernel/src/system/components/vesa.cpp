/*
http://www.petesqbsite.com/sections/tutorials/tuts/vbe3.pdf
*/

#include <system/components/vesa.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::core;

uint16_t* RealPTRToAddr(Real_Pointer ptr){
    return (uint16_t*)((ptr.B * 0x10) + ptr.A);
}

VESA::VESA(Virtual8086Manager* vm86)
: GraphicsDevice("VESA VBE Adapter"),
SystemComponent("VESA VBE", "VESA BIOS Extensions")
{ 
    this->virtual8086Manager = vm86;
}

VESAModeInfo* VESA::GetModeInfo(uint16_t mode)
{
    VESAModeInfo* info = (VESAModeInfo*)0x8000;
    MemoryOperations::memset(info, 0, sizeof(VESAModeInfo));

	VM86Arguments regs;
	MemoryOperations::memset(&regs, 0, sizeof(VM86Arguments));

	regs.AX = 0x4F01;
	regs.CX = mode;
	regs.DI = (uint16_t)0x8000;
	virtual8086Manager->CallInterrupt(0x10, &regs);	
	
	return info;
}

void VESA::SetVideoMode(uint16_t mode)
{
	VM86Arguments regs;
	MemoryOperations::memset(&regs, 0, sizeof(VM86Arguments));
	regs.AX = 0x4F02;
	regs.BX = mode;
	virtual8086Manager->CallInterrupt(0x10, &regs);
}

bool VESA::SelectBestVideoMode()
{
	Log(Info, "Initializing VESA Graphics Device");

	VESAControllerInfo* vesaInfo = (VESAControllerInfo*)0x7000;
	MemoryOperations::memset(vesaInfo, 0, 512);
	MemoryOperations::memcpy(vesaInfo->Signature, "VBE2", 4);
	
	Log(Info, "Probing For Controller Information");
	VM86Arguments regs;
	MemoryOperations::memset(&regs, 0, sizeof(VM86Arguments));
	regs.AX = 0x4F00;
	regs.DI = (uint16_t)0x7000;
	virtual8086Manager->CallInterrupt(0x10, &regs);

	if(MemoryOperations::memcmp(vesaInfo->Signature, "VESA", 4) == 0)
	{
		// Print Basic Info
		Log(Info, "VESA VBE Version: %d.%d", (vesaInfo->Version & 0xFF00) >> 8, vesaInfo->Version & 0x00FF);
		Log(Info, "OEM String: %s", (char*)RealPTRToAddr(vesaInfo->OemStringPtr));
		Log(Info, "Amount of Video Memory: %d Kb", vesaInfo->TotalMemory * 64);

		if((vesaInfo->Version & 0xFF00) >> 8 >= 2) { //VBE2.0+ Support some more info strings
			Log(Info, "Vendor: %s", (char*)RealPTRToAddr(vesaInfo->OemVendorNamePtr));
			Log(Info, "Product: %s", (char*)RealPTRToAddr(vesaInfo->OemProductNamePtr));
			Log(Info, "Revision: %s", (char*)RealPTRToAddr(vesaInfo->OemProductRevPtr));
		}

		// Check if EDID is loaded
		int preferedWidth = 0, preferedHeight = 0;
		bool edidValid = true;
		System::edid->PreferedMode(&preferedWidth, &preferedHeight);
		if(preferedWidth == 0 || preferedHeight == 0)
			edidValid = false;

		// Get mode array pointer
		uint16_t* modeArray = (uint16_t*)RealPTRToAddr(vesaInfo->VideoModePtr);
		if(edidValid)
			Log(Info, "Looping through video modes, using prefered mode %dx%d", preferedWidth, preferedHeight);
		else
			Log(Info, "Looping through video modes");

		///////////////
		// Select Best Video Mode from List
		///////////////
		uint16_t selectedModeNumber = 0;
		uint16_t fallbackModeNumber = 0;
		while(*modeArray != 0xFFFF) {
			VESAModeInfo* modeInfo = GetModeInfo(*modeArray);
			if((modeInfo->ModeAttributes & 0x90) != 0x90) { // Check for lineair framebuffer support + mode is graphics mode
				modeArray++;
				continue;
			}

			if(modeInfo->MemoryModel != 4 && modeInfo->MemoryModel != 6 ) { // Check if this is a packed pixel or direct color mode
				modeArray++;
				continue;
			}

			if(modeInfo->BitsPerPixel != DEFAULT_SCREEN_BPP) { // We only support 32-bit color atm
				modeArray++;
				continue;
			}
			
			// Valid Mode if we reach this
			// Now check resolution
			if(edidValid && modeInfo->XResolution == preferedWidth && modeInfo->YResolution == preferedHeight) { // We found the perfect mode for our monitor
				selectedModeNumber = *modeArray;
				break; // Stop the loop since we have found a perfect match
			}
			else if(modeInfo->XResolution == DEFAULT_SCREEN_WIDTH && modeInfo->YResolution == DEFAULT_SCREEN_HEIGHT) // We at least found a fallback resolution
				fallbackModeNumber = *modeArray;
			
			modeArray++;
		}

		if(selectedModeNumber == 0 && fallbackModeNumber != 0) { // We have not found prefered mode
			selectedModeNumber = fallbackModeNumber; // Use mode that should always work
			Log(Warning, "Using Fallback mode %w (%dx%d)", fallbackModeNumber, DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT);
		}

		if(selectedModeNumber != 0)
		{
			Log(Info, "Switching to video mode: %w", selectedModeNumber);
			this->SetVideoMode(selectedModeNumber);

			//Set the current mode info
			MemoryOperations::memcpy(&this->currentVideoMode, GetModeInfo(selectedModeNumber), sizeof(VESAModeInfo));

			Log(Info, "Framebuffer is at: %x", this->currentVideoMode.PhysBasePtr);

			//Store mode information to base class
			this->bpp = this->currentVideoMode.BitsPerPixel;
			this->height = this->currentVideoMode.YResolution;
			this->width = this->currentVideoMode.XResolution;
			this->framebufferPhys = this->currentVideoMode.PhysBasePtr;

			return true;
		}
		else // No valid mode found, not even a fallback resolution
		{
			Log(Error, "Could not find a usable video mode");
			return false;
		}
	}
	else
	{
		Log(Error, "Vesa info block does not have valid signature");
		return false;
	}
}