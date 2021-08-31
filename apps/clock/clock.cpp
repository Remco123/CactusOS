#include <new.h>
#include <gui/gui.h>
#include <gui/directgui.h>
#include <convert.h>
#include <time.h>
#include <datetime.h>
#include <string.h>
#include <math.h>
#include <log.h>
#include <proc.h>

int main(int argc, char** argv)
{
    GUI::SetDefaultFont();
    Context* mainScreen = GUI::RequestContext(160, 160, GUI::Width-160, 0);
    if(mainScreen == 0)
        return -1;

    mainScreen->sharedContextInfo->supportsTransparency = true;
    mainScreen->sharedContextInfo->background = true;
    mainScreen->sharedContextInfo->supportsDirtyRects = true;
    int prevSec = -1;
    while(1) {
        DateTime currentTime = DateTime::Current();
        if(currentTime.Seconds == prevSec)
            Time::Sleep(500);

        mainScreen->canvas->Clear(0x00395772);
        mainScreen->canvas->DrawFillCircle(0xFFDDDDDD, 160/2, 160/2 - 15, 60);
        mainScreen->canvas->DrawCircle(0xFF000000, 160/2, 160/2 - 15, 61);
        mainScreen->canvas->DrawString(GUI::defaultFont, currentTime.ToString(), 15, 140, 0xFF5200FF);

        double angleInDegrees, x, y = 0;

        // Seconds
        angleInDegrees = 360.0 * ((double)currentTime.Seconds/60.0) - 90.0;
        x = (double)(55 * Math::cos(angleInDegrees * MATH_PI / 180.0)) + 160/2;
        y = (double)(55 * Math::sin(angleInDegrees * MATH_PI / 180.0)) + 160/2 - 15;
        mainScreen->canvas->DrawLine(0xFF0000FF, 160/2, 160/2 - 15, x, y);

        // Minutes
        angleInDegrees = 360.0 * ((double)currentTime.Minutes/60.0) - 90.0;
        x = (double)(45 * Math::cos(angleInDegrees * MATH_PI / 180.0)) + 160/2;
        y = (double)(45 * Math::sin(angleInDegrees * MATH_PI / 180.0)) + 160/2 - 15;
        mainScreen->canvas->DrawLine(0xFF00FF00, 160/2, 160/2 - 15, x, y);

        // Hours
        angleInDegrees = 360.0 * ((double)(currentTime.Hours%12)/12.0) - 90.0;
        x = (double)(30 * Math::cos(angleInDegrees * MATH_PI / 180.0)) + 160/2;
        y = (double)(30 * Math::sin(angleInDegrees * MATH_PI / 180.0)) + 160/2 - 15;
        mainScreen->canvas->DrawLine(0xFFFF0000, 160/2, 160/2 - 15, x, y);

        // Numbers
        for(int i = 1; i <= 12; i++)
        {
            angleInDegrees = 360.0 * ((double)i/12.0) - 90.0;
            x = (double)(50 * Math::cos(angleInDegrees * MATH_PI / 180.0)) + 160/2;
            y = (double)(50 * Math::sin(angleInDegrees * MATH_PI / 180.0)) + 160/2 - 15;

            mainScreen->canvas->DrawString(GUI::defaultFont, Convert::IntToString(i), i < 10 ? x - 3 : x - 7, y - 8, 0xFF0000FF);
        }

        // Force paint
        mainScreen->sharedContextInfo->AddDirtyArea(0, 0, 160, 160);

        prevSec = currentTime.Seconds;
    }

    return 0;
}