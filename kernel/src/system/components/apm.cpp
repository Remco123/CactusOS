#include <system/components/apm.h>

#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

APMController::APMController()
: SystemComponent("APM Controller", "Controls Advanced Power Management via BIOS calls")
{
    Log(Info, "Initializing APM Controller");
    this->Enabled = false;

    // Check if APM is even availible
    VM86Arguments args;
    MemoryOperations::memset(&args, 0, sizeof(VM86Arguments));
    args.AX = (uint16_t)0x5300 | (uint16_t)APM_FUNC_INSTALL_CHECK;
    System::vm86Manager->CallInterrupt(0x15, &args);

    if(args.BX == APM_SIGNATURE)
        Log(Info, "APM %d.%d Present", (args.AX & 0xFF00) >> 8, (args.AX & 0x00FF));
    else {
        Log(Info, "APM not found on system, error = %b", (args.AX & 0xFF00) >> 8);
        return;
    }

    // Disconnect any interface
    MemoryOperations::memset(&args, 0, sizeof(VM86Arguments));
    args.AX = (uint16_t)0x5300 | (uint16_t)APM_FUNC_DISCONNECT;
    System::vm86Manager->CallInterrupt(0x15, &args);

    // Connect to APM BIOS Interface
    MemoryOperations::memset(&args, 0, sizeof(VM86Arguments));
    args.AX = (uint16_t)0x5300 | (uint16_t)APM_FUNC_CONNECT_REALMODE;
    System::vm86Manager->CallInterrupt(0x15, &args);

    // Set driver version to 1.2
    MemoryOperations::memset(&args, 0, sizeof(VM86Arguments));
    args.AX = (uint16_t)0x5300 | (uint16_t)APM_FUNC_DRIVER_VERSION;
    args.CX = 0x0102;
    System::vm86Manager->CallInterrupt(0x15, &args);
    if(args.AX != 0x0102) { //Error setting version 1.2
        Log(Error, "Could not set APM to version 1.2. Error = %b", (args.AX & 0xFF00) >> 8);
        return;
    }

    // Enable power management for all devices
    MemoryOperations::memset(&args, 0, sizeof(VM86Arguments));
    args.AX = (uint16_t)0x5300 | (uint16_t)APM_FUNC_ENABLE_POWER_MANAGEMENT;
    args.BX = APM_ALL_DEVICE; //All Devices
    args.CX = 0x1; //Enable power control by APM BIOS
    System::vm86Manager->CallInterrupt(0x15, &args);

    // Apply some OS specific settings
    DisableRingIndicator();
    DisableResumeTimer();

    // Indicate that power management is enabled
    this->Enabled = true;
}
void APMController::CheckAndHandleEvents()
{
    //Log(Info, "APM Event check");

    // Request Event info from BIOS
    VM86Arguments args;
    MemoryOperations::memset(&args, 0, sizeof(VM86Arguments));
    args.AX = (uint16_t)0x5300 | (uint16_t)APM_FUNC_GET_PM_EVENT;
    System::vm86Manager->CallInterrupt(0x15, &args);

    uint8_t errorCode = (args.AX & 0xFF00) >> 8;
    if(errorCode == 0x3 || errorCode == 0xB || errorCode == 0x80) // Error or no events
        return;
    
    uint16_t eventCode = args.BX;
    Log(Info, "APM Event %w", eventCode);

    switch(eventCode)
    {
        case 0x000A: //Suspend Request from user
            SetPowerState(APM_ALL_DEVICE, APM_POWER_REJECT); //TODO: Implement proper suspend. Right now system wakes up again after a few seconds.
            break;
        case 0x0003: //Normal Resume System Notification
            
            break;
        default:
            break;
    }


    // Check for remaining events until error
    CheckAndHandleEvents();
}
void APMController::SetPowerState(uint16_t device, uint8_t state)
{
    VM86Arguments args;
    MemoryOperations::memset(&args, 0, sizeof(VM86Arguments));
    args.AX = (uint16_t)0x5300 | (uint16_t)APM_FUNC_SET_POWER_STATE;
    args.BX = device;
    args.CX = state;
    System::vm86Manager->CallInterrupt(0x15, &args);
}
void APMController::DisableResumeTimer()
{
    VM86Arguments args;
    MemoryOperations::memset(&args, 0, sizeof(VM86Arguments));
    args.AX = (uint16_t)0x5300 | (uint16_t)APM_FUNC_SET_RESUME_TIMER;
    System::vm86Manager->CallInterrupt(0x15, &args);
}
void APMController::DisableRingIndicator()
{
    VM86Arguments args;
    MemoryOperations::memset(&args, 0, sizeof(VM86Arguments));
    args.AX = (uint16_t)0x5300 | (uint16_t)APM_FUNC_SET_RESUME_RING;
    System::vm86Manager->CallInterrupt(0x15, &args);
}