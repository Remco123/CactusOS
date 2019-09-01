#ifndef TERMINAL_CONTROL_H
#define TERMINAL_CONTROL_H

#include <gui/widgets/control.h>

#define TERM_WIDTH 37
#define TERM_HEIGH 12

class TerminalControl : public Control
{
private:
    char* textBuffer = 0;
    char lastInputKey = 0;

    int x,y;
public:
    uint32_t textColor = 0xFF000000;

    TerminalControl(int w, int h);
    ~TerminalControl();

    // Read a new command from this command prompt.
    char* ReadCommand(char* prompt = 0);
    void Write(char c);
    void Write(char* str);
    void ScrollLine();
    void Clear();

    void DrawTo(Canvas* context, int x_abs, int y_abs) override;
    
/*/////////
// Events called by parent or context
*//////////
friend class Window;
friend class Context;
protected:
    // Called on keypress 
    void OnKeyPress(char key) override;
};

#endif