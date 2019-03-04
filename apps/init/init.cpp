#include <log.h>
#include <api.h>
#include <types.h>
#include <syscall.h>
#include <gui/directgui.h>

using namespace LIBCactusOS;

int main()
{
    API::Initialize();

    Log(LogLevel::Info, "Init process started!");

    if (DirectGUI::RequestFramebuffer() == SYSCALL_RET_SUCCES)
    {
        uint8_t c = 0x00;
        uint8_t spd = 0;
        uint32_t* buf = (uint32_t*)(DIRECT_GUI_ADDR);
        while (1)
        {
            for (uint32_t i = 0; i < 1024 * 768; i++)
                buf[i] = c++ ^ i;
            c+= (spd % 100) ? spd : spd++;
        }
    }

    return 0;
}