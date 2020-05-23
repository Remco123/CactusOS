#include <system/components/edid.h>

#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

uint8_t* edidBuffer = 0;

EDID::EDID()
: SystemComponent("EDID", "Mechanism to acquire monitor information")
{
    edidBuffer = 0;
}

bool EDIDCheckChecksum()
{
    const char* data = (const char *)edidBuffer;
    char checksum = 0;

    /* Check EDID checksum.  */
    for (int i = 0; i < 128; ++i)
        checksum += data[i];

    if (checksum != 0)
        return false;

    return true;
}

void EDID::AcquireEDID()
{
    VM86Arguments args;
    MemoryOperations::memset(&args, 0, sizeof(VM86Arguments));

    Log(Info, "Checking if EDID is supported");
    args.AX = 0x4F15;
    System::vm86Manager->CallInterrupt(0x10, &args);

    if(args.AX != EDID_SUCCES) {
        Log(Info, "EDID Not supported, status = %w", args.AX);
        return;
    }

    // Assign EDID buffer to area of memory within first 1MB
    edidBuffer = (uint8_t*)0x7500;
    MemoryOperations::memset(edidBuffer, 0, 128);
    MemoryOperations::memset(&args, 0, sizeof(VM86Arguments));

    args.AX = 0x4F15;
    args.BX = 0x0001;
    args.DI = 0x7500;

    Log(Info, "Reading EDID via BIOS/VBE");
    System::vm86Manager->CallInterrupt(0x10, &args);
    
    if((args.AX & 0x00FF) != 0x4F) {
        Log(Info, "Read EDID Failed, status = %w", args.AX);
        edidBuffer = 0; //Reset buffer
        return;
    }

    Log(Info, "Checking if EDID makes sense");
    if(EDIDCheckChecksum() == false) {
        Log(Info, "EDID Checksum incorrect");
        edidBuffer = 0; //Reset buffer
        return;
    }

    const uint8_t edidHeader[] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
    if(MemoryOperations::memcmp(edidBuffer, edidHeader, 8) != 0) {
        Log(Info, "EDID Header incorrect");
        edidBuffer = 0; //Reset buffer
        return;
    }

    //////////////
    // Start Parsing Data
    //////////////
    EDIDInfoBlock* info = (EDIDInfoBlock*)edidBuffer;
    Log(Info, "EDID Structure Version %d.%d", info->version, info->revision);
    for(int i = 0; i < 4; i++) { //Loop through descriptors
        switch(info->detailed_timings[i].dataType) {
            case 0xFF: //Monitor Serial Number - Stored as ASCII
                Log(Info, "Monitor Serial: %s", info->detailed_timings[i].descriptorData);
                break;
            case 0xFE: //ASCII String - Stored as ASCII
                Log(Info, "ASCII String: %s", info->detailed_timings[i].descriptorData);
                break;
            case 0xFC: //Monitor Name, stored as ASCII
                Log(Info, "Monitor Name: %s", info->detailed_timings[i].descriptorData);
                break;
            default:
                break;
        }
    }
}

void EDID::PreferedMode(int* widthPtr, int* heightPtr)
{
    if(edidBuffer == 0) {
        Log(Info, "EDID: Could not find prefered mode since edid is not loaded");
        return;
    }
    
    EDIDInfoBlock* info = (EDIDInfoBlock*)edidBuffer;

    if (info->version == 1 && (info->feature_support & (1<<1)) && info->detailed_timings[0].flag != 0)
    {
        TimingsInfoBlock* timing = (TimingsInfoBlock*)(info->detailed_timings);

        *widthPtr = timing->horizontal_active_lo | (((unsigned int)(timing->horizontal_hi & 0xF0)) << 4);
        *heightPtr = timing->vertical_active_lo | (((unsigned int)(timing->vertical_hi & 0xF0)) << 4);
    }

    if(*widthPtr == 0 || *heightPtr == 0)
        Log(Info, "EDID: Could not find prefered mode, error parsing timings");
}