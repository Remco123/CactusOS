#ifndef TERMINAL_CONTROL_H
#define TERMINAL_CONTROL_H

#include <gui/widgets/control.h>

#define TERM_WIDTH 74
#define TERM_HEIGH 26

class TerminalControl : public Control
{
private:
    char* textBuffer = 0;
    List<char> inputKeys;

    int x,y;
    bool cursor = false;
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
    void ToggleCursor();

    void DrawTo(Canvas* context, int x_abs, int y_abs) override;
    
/*/////////
// Events called by parent or context
*//////////
friend class Window;
friend class Context;
protected:
    // Called on keypress 
    void OnKeyDown(uint8_t key, KEYPACKET_FLAGS modifiers) override;
};

#endif