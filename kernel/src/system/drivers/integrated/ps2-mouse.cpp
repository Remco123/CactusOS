#include <system/drivers/integrated/ps2-mouse.h>
#include <core/port.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;
using namespace CactusOS::core;

PS2MouseDriver::PS2MouseDriver()
: InterruptHandler(0x2C), Driver("PS2 Mouse", "Driver for a generic ps2 mouse")
{
    this->packetBuffer = new signed char[3];
}

/**
 * Wait for the mouse to become ready for a read or write opperation
*/
inline void mouseWait(bool type)
{
  uint32_t timeOut = 100000;
  if(type == false)
  {
    while(timeOut--)
      if((inportb(MOUSE_COMMAND) & 1) == 1)
        return;

    Log(Warning, "Mouse wait timeout type=%d", type);
    return;
  }
  else
  {
    while(timeOut--)
      if((inportb(MOUSE_COMMAND) & 2) == 0)
        return;

    Log(Warning, "Mouse wait timeout type=%d", type);
    return;
  }
}
/**
 * Write a byte to the mouse
*/
inline void mouseWrite(uint8_t a_write)
{
  //Wait to be able to send a command
  mouseWait(1);
  //Tell the mouse we are sending a command
  outportb(0x64, 0xD4);
  //Wait for the final part
  mouseWait(1);
  //Finally write
  outportb(0x60, a_write);
}
/**
 * Read a byte from the mouse
*/
inline uint8_t mouseRead()
{
  //Get's response from mouse
  mouseWait(0);
  return inportb(0x60);
}

bool PS2MouseDriver::Initialize()
{
    //Enable the auxiliary mouse device
    mouseWait(1);
    outportb(0x64, 0xA8);
    
    //Enable the interrupts
    mouseWait(1);
    outportb(0x64, 0x20);

    mouseWait(0);
    uint8_t status = (inportb(0x60) | 2);

    mouseWait(1);
    outportb(0x64, 0x60);

    mouseWait(1);
    outportb(0x60, status);
    
    //Tell the mouse to use default settings
    mouseWrite(0xF6);
    if(mouseRead() != MOUSE_ACK)  //Acknowledge
        return false;
    
    //Enable the mouse
    mouseWrite(0xF4);
    if(mouseRead() != MOUSE_ACK)  //Acknowledge
        return false;

    //Enable scroll wheel if present
    if(EnableScrollWheel())
        BootConsole::WriteLine("PS2 Mouse scrollwheel enabled");
    else
        BootConsole::WriteLine("PS2 Mouse has no scrollwheel");

    this->ready = true;
    return true;
}
bool PS2MouseDriver::EnableScrollWheel()
{
    if(MouseID == 3) //We are already using the scrollwheel
        return true;
    
    if(!SetSampleRate(200) || !SetSampleRate(100) || !SetSampleRate(80))
        return false;
    
    mouseWrite(0xF2);
    if(mouseRead() != MOUSE_ACK)  //Acknowledge
        return false;
    
    //Read new ID
    uint8_t newID = mouseRead();

    if(newID != 3)
        return false;
    
    //Create new buffer
    delete this->packetBuffer;
    this->packetBuffer = new signed char[4];

    MouseID = newID;
    return true;
}
bool PS2MouseDriver::SetSampleRate(uint8_t value)
{
    mouseWrite(0xF3);
    if(mouseRead() != MOUSE_ACK)  //Acknowledge
        return false;
    
    mouseWrite(value);
    if(mouseRead() != MOUSE_ACK)  //Acknowledge
        return false;
    
    return true;
}
uint32_t PS2MouseDriver::HandleInterrupt(uint32_t esp)
{
    if(!this->ready)
        return esp;
    
    uint8_t status = inportb(MOUSE_COMMAND);
    if (!(status & 0x20))
        return esp;

    uint8_t dataByte = inportb(MOUSE_DATA);

    switch(MouseCycle)
    {
    case 0:
        {
            packetBuffer[0] = dataByte;
            MouseCycle++;
        }
        break;
    case 1:
        {
            packetBuffer[1] = dataByte;
            MouseCycle++;
        }
        break;
    case 2:
        {
            packetBuffer[2] = dataByte;

            if(MouseID == 0) //End of packed
            {
                ProcessPacket();
                MouseCycle = 0;
            }
            else
                MouseCycle++;
        }
        break;
    case 3:
        {
            packetBuffer[3] = dataByte;
            ProcessPacket();
            MouseCycle = 0;
        }
        break;
    }

    return esp;
}
void PS2MouseDriver::ProcessPacket()
{
    MousePacket* packet = (MousePacket*)this->packetBuffer;

    int realX = packet->XMovement - (packet->Xsign & 0x100);
    int realY = packet->YMovement - (packet->Ysign & 0x100);

    //Log(Info, "Mouse: X=%d Y=%d Z=%d LB=%d RB=%d MB=%d", realX, realY, (this->MouseID == 3 ? packet->ZMovement : 0), packet->LeftBTN, packet->RightBTN, packet->MiddleBTN);

    //Upload values to systeminfo
    System::systemInfo->MouseLeftButton = packet->LeftBTN;
    System::systemInfo->MouseMiddleButton = packet->MiddleBTN;
    System::systemInfo->MouseRightButton = packet->RightBTN;
    System::systemInfo->MouseZ += (int8_t)(this->MouseID == 3 ? packet->ZMovement : 0);

    int newX = (System::systemInfo->MouseX + realX);
    if((newX >= 0) && (newX < (int)System::gfxDevice->width))
        System::systemInfo->MouseX = newX;
    else if(newX < 0)
        System::systemInfo->MouseX = 0;
    else if(newX >= (int)System::gfxDevice->width)
        System::systemInfo->MouseX = System::gfxDevice->width - 1;
    
    int newY = (System::systemInfo->MouseY - realY);
    if((newY >= 0) && (newY < (int)System::gfxDevice->height))
        System::systemInfo->MouseY = newY;
    else if(newY < 0)
        System::systemInfo->MouseY = 0;
    else if(newY >= (int)System::gfxDevice->height)
        System::systemInfo->MouseY = System::gfxDevice->height - 1;
}