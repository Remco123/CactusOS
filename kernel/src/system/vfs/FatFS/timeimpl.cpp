#include "ff.h"

#include <system/system.h>

using namespace CactusOS::system;

extern "C" DWORD GetFATTime()
{
    return ((DWORD)(System::rtc->GetYear() - 1980) << 25 | (DWORD)System::rtc->GetMonth() << 21 | (DWORD)System::rtc->GetDay() << 16);
}