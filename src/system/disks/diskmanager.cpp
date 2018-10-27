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
    this->pit = pit; //We need this for benchmarking

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

void DiskManager::EjectDrive(common::uint32_t driveNumber)
{
    if(driveNumber < this->numDisks && this->allDisks[driveNumber] != 0)
    {
        if(this->allDisks[driveNumber]->type == DiskType::CD && String::strcmp(this->allDisks[driveNumber]->controller->controllerID, "IDE"))
        {
            IDEController* controller = (IDEController*) this->allDisks[driveNumber]->controller;
            printf("Ejecting IDE Drive: "); printf(Convert::IntToString(this->allDisks[driveNumber]->diskNumber)); printf("\n");
            controller->EjectDrive(this->allDisks[driveNumber]->diskNumber);
        }
    }
}
void DiskManager::BenchmarkDrive(common::uint32_t driveNumber)
{
    if(driveNumber < this->numDisks && this->allDisks[driveNumber] != 0)
    {
        printf("Benchmarking disk: "); printf(Convert::IntToString(driveNumber)); printf("\n");
        uint8_t* buffer = new uint8_t[2048];
        
        printf("Read test");
        double results[10];
        int sector = 0;
        for(int i = 0; i < 10; i++)
        {    
            printf(".");
            uint32_t startTicks = pit->Ticks();
            for(int read = 0; read < 100; read++)
            {
                this->allDisks[driveNumber]->ReadSector(sector++, buffer);
            }
            results[i] = pit->Ticks() - startTicks;
        }
        printf("\n");
        double average = (double)(results[0] + results[1] + results[2] + results[3] + results[4] + results[5] + results[6] + results[7] + results[8] + results[9]) / (double)10;
        
        int sectorSize = 512;
        if(this->allDisks[driveNumber]->type == DiskType::CD && String::strcmp(this->allDisks[driveNumber]->controller->controllerID, "IDE"))
            sectorSize = 2048;
        
        //average contains the amount of ms it takes to read 100 sectors
        double perSector = average / 100;
        double perByte = perSector / sectorSize;

        printf("Speed: "); printf(Convert::IntToString(1000 / (perByte * 1024 * 1024))); printf(" Mb/s\n");

        delete buffer;
    }
}