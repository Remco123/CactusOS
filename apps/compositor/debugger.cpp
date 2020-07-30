#include "debugger.h"
#include <convert.h>
#include <time.h>
#include <gui/gui.h>

using namespace LIBCactusOS;

CompositorDebugger::CompositorDebugger(Compositor* target)
{
    this->target = target;
}


void CompositorDebugger::ProcessContext(ContextInfo* ctx)
{
    if(ctx->background)
        return;
    
    this->target->backBufferCanvas->DrawString(GUI::defaultFont, Convert::IntToString(ctx->width), ctx->x + ctx->width/2 - 30, ctx->y + ctx->height - 20, 0xFF0000FF);
    this->target->backBufferCanvas->DrawString(GUI::defaultFont, Convert::IntToString(ctx->height), ctx->x + 5, ctx->y + ctx->height/2 - 30, 0xFF0000FF);

    // Grey Box
    this->target->backBufferCanvas->DrawFillRect(0xFFAAAAAA, ctx->x + ctx->width - 200, ctx->y + 30, 200, 200);

    // Framebuffer address info server
    this->target->backBufferCanvas->DrawString(GUI::defaultFont, "Addr,Svr: 0x", ctx->x + ctx->width - 200 + 5, ctx->y + 30 + 5, 0xFF0000FF);
    this->target->backBufferCanvas->DrawString(GUI::defaultFont, Convert::IntToHexString(ctx->virtAddrServer), ctx->x + ctx->width - 100, ctx->y + 30 + 5, 0xFF0000FF);

    // Framebuffer address info client
    this->target->backBufferCanvas->DrawString(GUI::defaultFont, "Addr,Clt: 0x", ctx->x + ctx->width - 200 + 5, ctx->y + 30 + 25, 0xFF0000FF);
    this->target->backBufferCanvas->DrawString(GUI::defaultFont, Convert::IntToHexString(ctx->virtAddrClient), ctx->x + ctx->width - 100, ctx->y + 30 + 25, 0xFF0000FF);

    // ID Info
    this->target->backBufferCanvas->DrawString(GUI::defaultFont, "ID: ", ctx->x + ctx->width - 200 + 5, ctx->y + 30 + 45, 0xFF0000FF);
    this->target->backBufferCanvas->DrawString(GUI::defaultFont, Convert::IntToString(ctx->id), ctx->x + ctx->width - 160, ctx->y + 30 + 45, 0xFF0000FF);

    // Proc Info
    this->target->backBufferCanvas->DrawString(GUI::defaultFont, "PID: ", ctx->x + ctx->width - 200 + 5, ctx->y + 30 + 65, 0xFF0000FF);
    this->target->backBufferCanvas->DrawString(GUI::defaultFont, Convert::IntToString(ctx->clientID), ctx->x + ctx->width - 150, ctx->y + 30 + 65, 0xFF0000FF);
}

void CompositorDebugger::ProcessGeneral()
{
    static uint64_t startFrameTime = 0;
    static uint32_t frameCount = 0;
    static int fps = 60;

    if(startFrameTime == 0)
        startFrameTime = Time::Ticks();

    // Increase framecount by 1
    frameCount++;

    if(Time::Ticks() - startFrameTime >= 1000) // 1 sec had passed
    {
        // Reset frametime
        startFrameTime = 0;

        // Calculate fps
        fps = frameCount;

        // Reset framecount
        frameCount = 0;
    }

    this->target->backBufferCanvas->DrawFillRect(0xFFAAAAAA, 0, 0, 50, 25);
    this->target->backBufferCanvas->DrawString(GUI::defaultFont, Convert::IntToString(fps), 5, 5, 0xFF0000FF);
}
