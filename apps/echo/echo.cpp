#include <new.h>
#include <gui/gui.h>
#include <gui/widgets/button.h>
#include <gui/widgets/control.h>
#include <gui/widgets/window.h>
#include <gui/widgets/label.h>
#include <gui/directgui.h>
#include <convert.h>
#include <string.h>
#include <log.h>
#include <proc.h>
#include <time.h>

using namespace LIBCactusOS;

int main()
{
    Print("Echo application\n");
    for(int i = 0; i < 5; i++)
        Print("_****************_\n");
    
    return 0;
}