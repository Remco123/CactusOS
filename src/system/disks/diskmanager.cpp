#include <system/disks/diskmanager.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::core;

#include <system/disks/controllers/fdc.h>
#include <system/disks/controllers/ide.h>

void printf(char*);

DiskManager::DiskManager()
{
    this->numControllers = 0;
    this->numDisks = 0;
}
            
void DiskManager::DetectAndLoadDisks(InterruptManager* interrupts, PIT* pit)
{
    printf("Detecting Disks\n");
    IDEController* ideController = new IDEController(interrupts);
    ideController->InitIDE(0x1F0, 0x3F4, 0x170, 0x374, 0x000, pit);
    this->controllers[this->numControllers++] = (DiskController*)ideController;
    
    FloppyDiskController* floppyController = new FloppyDiskController(interrupts);
    floppyController->InitController();
    this->controllers[this->numControllers++] = (DiskController*)floppyController;


    for(int i = 0; i < this->numControllers; i++)
        this->controllers[i]->AsignDisks(this);

    printf("Found: "); printf(Convert::IntToString(this->numDisks)); printf(" disks in total\n");
}