#include <new.h>
#include <log.h>
#include <gui/widgets/button.h>
#include <gui/widgets/window.h>
#include <gui/directgui.h>
#include <random.h>
#include <datetime.h>
#include <time.h>
#include <convert.h>
#include <string.h>
#include <gui/widgets/label.h>

using namespace LIBCactusOS;

#define BLOCK_W 30
#define BLOCK_H 30
#define ENTRY(x,y) (y*fieldWidth+x)

class Block : public Button
{
public:
    bool open;
    bool mine;
    bool flaged;

    int fx;
    int fy;

    Block() : Button() {
        open = false;
        mine = false;
        flaged = false;

        this->width = BLOCK_W;
        this->height = BLOCK_H;
    }

    void Opened()
    {
        this->backColor = 0xFFDDDDDD;
    }

    void Flagged(bool flag)
    {
        this->backColor = flag ? 0xFF88FFAA : 0xFF190A39;
    }
};

const int fieldWidth = 10;
const int fieldHeight = 10;
const int numOfMines = 16;

enum GameStatusOptions
{
    Bussy,
    Won,
    Lose
};

GameStatusOptions gameStatus = Bussy;

Block** BlockList;
bool firstClick = true;

bool IsMine(int x, int y)
{
    if(x >= 0 && x < fieldWidth && y >= 0 && y < fieldHeight)
        return BlockList[ENTRY(x,y)]->mine;
    
    return false;
}

Block* GetBlock(int x, int y)
{
    if(x >= 0 && x < fieldWidth && y >= 0 && y < fieldHeight)
        return BlockList[ENTRY(x,y)];
    
    return 0;
}

void ToggleFlag(Block* block)
{
    block->flaged = !block->flaged;
    if(block->flaged)
        block->label = "F";
    else
        block->label = "";
    
    block->Flagged(block->flaged);
}

int CountMinesAround(Block* src)
{
    int result = 0;

    if(IsMine(src->fx - 1, src->fy + 1))
        result++;
    if(IsMine(src->fx, src->fy + 1))
        result++;
    if(IsMine(src->fx + 1, src->fy + 1))
        result++;

    if(IsMine(src->fx - 1, src->fy))
        result++;
    if(IsMine(src->fx + 1, src->fy))
        result++;

    if(IsMine(src->fx - 1, src->fy - 1))
        result++;
    if(IsMine(src->fx, src->fy - 1))
        result++;
    if(IsMine(src->fx + 1, src->fy - 1))
        result++;

    return result; 
}

bool IsBlockAround(Block* middleBlock, Block* checkWith)
{
    int x = middleBlock->fx;
    int y = middleBlock->fy;

    if(GetBlock(x - 1, y + 1) == checkWith)
        return true;
    if(GetBlock(x, y + 1) == checkWith)
        return true;
    if(GetBlock(x + 1, y + 1) == checkWith)
        return true;

    if(GetBlock(x - 1, y) == checkWith)
        return true;
    if(GetBlock(x + 1, y) == checkWith)
        return true;

    if(GetBlock(x - 1, y - 1) == checkWith)
        return true;
    if(GetBlock(x, y - 1) == checkWith)
        return true;
    if(GetBlock(x + 1, y - 1) == checkWith)
        return true;
    
    return false;
}

void Open(Block* block)
{
    if(block != 0 && block->open == false)
    {
        block->open = true;
        block->Opened();
        if(block->mine)
            gameStatus = Lose;
        else
        {
            int minesAround = CountMinesAround(block);
            if(minesAround == 0) {
                Open(GetBlock(block->fx - 1, block->fy + 1));
                Open(GetBlock(block->fx, block->fy + 1));
                Open(GetBlock(block->fx + 1, block->fy + 1));

                Open(GetBlock(block->fx - 1, block->fy));
                Open(GetBlock(block->fx + 1, block->fy));

                Open(GetBlock(block->fx - 1, block->fy - 1));
                Open(GetBlock(block->fx, block->fy - 1));
                Open(GetBlock(block->fx + 1, block->fy - 1));
            }
            else
            {
                block->label = new char[2];
                block->label[0] = Convert::IntToString(minesAround)[0];
                block->label[1] = '\0';
            }
        }
    }
}

void HandleFirstClick(Block* srcBlock)
{
    Random::SetSeed(DateTime::Current().Seconds * (uint32_t)Time::Ticks());

    int mineCount = 0;
    while(mineCount < numOfMines)
    {
        int rx = Random::Next(0, fieldWidth-1);
        int ry = Random::Next(0, fieldHeight-1);

        Block* target = GetBlock(rx, ry);
        if(target != srcBlock && target->mine == false && !IsBlockAround(srcBlock, target))
        {
            target->mine = true;
            //target->label = "M";
            mineCount++;
        }
    }
}

void BlockClickHandler(void* sender, MouseButtonArgs args)
{
    Block* block = (Block*)sender;
    if(block->open)
        block->Opened();
    else
        block->Flagged(block->flaged);
    
    if(firstClick && args.button == MouseButtons::Left) {
        HandleFirstClick(block);
        firstClick = false;
        Open(block);
    }
    else if(firstClick == false)
    {
        if(args.button == MouseButtons::Left)
        {
            Open(block);

            int openBlocks = 0;
            for(int i = 0; i < fieldWidth*fieldHeight; i++)
                if(BlockList[i]->open)
                    openBlocks++;
            
            if(openBlocks == ((fieldWidth*fieldHeight) - numOfMines))
                gameStatus = Won;
        }
        else if(args.button == MouseButtons::Right && block->open == false)
            ToggleFlag(block);
    }
}

int main(int argc, char** argv)
{
    GUI::SetDefaultFont();

    Print("Starting New Mines Game...\n");
    BlockList = new Block*[fieldWidth*fieldHeight];
    firstClick = true;
    gameStatus = Bussy;
    
    Window* mainWindow = new Window(300, 330, GUI::Width/2 - 150, GUI::Width/2 - 165);
    mainWindow->titleString = "CactusOS Mines Game";
    
    for(int placeY = 0; placeY < fieldHeight; placeY++)
        for(int placeX = 0; placeX < fieldWidth; placeX++)
        {
            Block* block = new Block();
            block->x = placeX * BLOCK_W;
            block->y = placeY * BLOCK_H;

            block->fx = placeX;
            block->fy = placeY;
            
            block->MouseClick += BlockClickHandler;
            BlockList[ENTRY(placeX,placeY)] = block;
            mainWindow->AddChild(block);
        }

    while(GUI::HasItems() && gameStatus == Bussy)
    {
        GUI::DrawGUI();
        GUI::ProcessEvents();
    }

    if(gameStatus == Bussy)
        return 0;
    
    mainWindow->Close();

    Window* resultWindow = new Window(200, 100, GUI::Width/2 - 100, GUI::Width/2 - 50);
    resultWindow->titleString = "Game Finished!";

    Label* label = new Label(gameStatus == Won ? (char*)"You completed the game!" : (char*)"You have lost!");
    resultWindow->AddChild(label);

    while(GUI::HasItems())
    {
        GUI::DrawGUI();
        GUI::ProcessEvents();
    }
    
    return 0;
}