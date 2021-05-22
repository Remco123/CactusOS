#include <gui/property.h>
#include <gui/widgets/control.h>

using namespace LIBCactusOS;

void LIBCactusOS::UpdateGUIPropertyTargetGUI(Control* target)
{
    if(target) target->ForcePaint();
}