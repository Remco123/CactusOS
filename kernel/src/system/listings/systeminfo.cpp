#include <system/listings/systeminfo.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

bool SystemInfoManager::HandleSysinfoRequest(void* arrayPointer, int count, common::uint32_t retAddr, bool getSize)
{
    LIBCactusOS::SIPropertyProvider* items = (LIBCactusOS::SIPropertyProvider*)arrayPointer;

    if(items[0].type != LIBCactusOS::SIPropertyIdentifier::String || !items[0].id)
        return false; // First identifier needs to be a string

    if(String::strcmp(items[0].id, "properties"))
    {
        if(items[1].type != LIBCactusOS::SIPropertyIdentifier::String || !items[1].id)
            return false; // Second identifier needs to be a string
        
        if(String::strcmp(items[1].id, "disks")) {
            if(getSize) {
                *((int*)retAddr) = System::diskManager->allDisks.size();
                return true;
            }
            if(items[2].type != LIBCactusOS::SIPropertyIdentifier::Index || items[3].type != LIBCactusOS::SIPropertyIdentifier::String)
                return false; // Needs to be index for collection and next needs to be property id

            int index = items[2].index;
            if(String::strcmp(items[3].id, "size")) {
                *((uint64_t*)retAddr) = System::diskManager->allDisks[index]->size;
                return true;
            }
            else if(String::strcmp(items[3].id, "identifier")) {
                char* targ = (char*)retAddr;
                int len = String::strlen(System::diskManager->allDisks[index]->identifier);
                MemoryOperations::memcpy(targ, System::diskManager->allDisks[index]->identifier, len);
                targ[len] = '\0';
                return true;
            }
            else if(String::strcmp(items[3].id, "blocks")) {
                *((uint32_t*)retAddr) = System::diskManager->allDisks[index]->numBlocks;
                return true;
            }
            else if(String::strcmp(items[3].id, "blocksize")) {
                *((uint32_t*)retAddr) = System::diskManager->allDisks[index]->blockSize;
                return true;
            }
            else if(String::strcmp(items[3].id, "type")) {
                *((int*)retAddr) = (int)System::diskManager->allDisks[index]->type;
                return true;
            }
            else
                return false;         
        }
        else if(String::strcmp(items[1].id, "usbdevices")) {
            if(getSize) {
                *((int*)retAddr) = System::usbManager->deviceList.size();
                return true;
            }
            if(items[2].type != LIBCactusOS::SIPropertyIdentifier::Index || items[3].type != LIBCactusOS::SIPropertyIdentifier::String)
                return false; // Needs to be index for collection and next needs to be property id

            int index = items[2].index;
            if(String::strcmp(items[3].id, "name")) {
                char* targ = (char*)retAddr;
                int len = String::strlen(System::usbManager->deviceList[index]->deviceName);
                MemoryOperations::memcpy(targ, System::usbManager->deviceList[index]->deviceName, len);
                targ[len] = '\0';
                return true;
            }
            else if(String::strcmp(items[3].id, "address")) {
                *((int*)retAddr) = (int)System::usbManager->deviceList[index]->devAddress;
                return true;
            }
            else if(String::strcmp(items[3].id, "controller")) {
                *((int*)retAddr) = (int)System::usbManager->controllerList.IndexOf(System::usbManager->deviceList[index]->controller);
                return true;
            }
            else if(String::strcmp(items[3].id, "prodID")) {
                *((uint16_t*)retAddr) = (int)System::usbManager->deviceList[index]->productID;
                return true;
            }
            else if(String::strcmp(items[3].id, "vendID")) {
                *((uint16_t*)retAddr) = (int)System::usbManager->deviceList[index]->vendorID;
                return true;
            }
            else if(String::strcmp(items[3].id, "endpoints")) {
                if(getSize) {
                    *((int*)retAddr) = System::usbManager->deviceList[index]->endpoints.size();
                    return true;
                }
                //TODO: Add more properties here
            }
            else
                return false;            
        }
        else if(String::strcmp(items[1].id, "usbcontrollers")) {
            if(getSize) {
                *((int*)retAddr) = System::usbManager->controllerList.size();
                return true;
            }
            if(items[2].type != LIBCactusOS::SIPropertyIdentifier::Index || items[3].type != LIBCactusOS::SIPropertyIdentifier::String)
                return false; // Needs to be index for collection and next needs to be property id

            int index = items[2].index;
            if(String::strcmp(items[3].id, "type")) {
                *((int*)retAddr) = System::usbManager->controllerList[index]->type;
                return true;
            }
            else
                return false;         
        }
        else if(String::strcmp(items[1].id, "pcidevs")) {
            if(getSize) {
                *((int*)retAddr) = System::pci->deviceList.size();
                return true;
            }
            if(items[2].type != LIBCactusOS::SIPropertyIdentifier::Index || items[3].type != LIBCactusOS::SIPropertyIdentifier::String)
                return false; // Needs to be index for collection and next needs to be property id

            int index = items[2].index;
            if(String::strcmp(items[3].id, "bus")) {
                *((uint16_t*)retAddr) = System::pci->deviceList[index]->bus;
                return true;
            }
            else if(String::strcmp(items[3].id, "classID")) {
                *((uint8_t*)retAddr) = System::pci->deviceList[index]->classID;
                return true;
            }
            else if(String::strcmp(items[3].id, "device")) {
                *((uint16_t*)retAddr) = System::pci->deviceList[index]->device;
                return true;
            }
            else if(String::strcmp(items[3].id, "deviceID")) {
                *((uint16_t*)retAddr) = System::pci->deviceList[index]->deviceID;
                return true;
            }
            else if(String::strcmp(items[3].id, "function")) {
                *((uint16_t*)retAddr) = System::pci->deviceList[index]->function;
                return true;
            }
            else if(String::strcmp(items[3].id, "interrupt")) {
                *((uint8_t*)retAddr) = System::pci->deviceList[index]->interrupt;
                return true;
            }
            else if(String::strcmp(items[3].id, "progIF")) {
                *((uint8_t*)retAddr) = System::pci->deviceList[index]->programmingInterfaceID;
                return true;
            }
            else if(String::strcmp(items[3].id, "revisionID")) {
                *((uint8_t*)retAddr) = System::pci->deviceList[index]->revisionID;
                return true;
            }
            else if(String::strcmp(items[3].id, "subclassID")) {
                *((uint8_t*)retAddr) = System::pci->deviceList[index]->subclassID;
                return true;
            }
            else if(String::strcmp(items[3].id, "vendorID")) {
                *((uint16_t*)retAddr) = System::pci->deviceList[index]->vendorID;
                return true;
            }
            else
                return false;         
        }
        else if(String::strcmp(items[1].id, "gfxdevice")) {
            if(items[2].type != LIBCactusOS::SIPropertyIdentifier::String)
                return false; // Needs to be property id
            
            if(String::strcmp(items[2].id, "bpp")) {
                *((uint8_t*)retAddr) = System::gfxDevice->bpp;
                return true;
            }
            else if(String::strcmp(items[2].id, "width")) {
                *((uint32_t*)retAddr) = System::gfxDevice->width;
                return true;
            }
            else if(String::strcmp(items[2].id, "height")) {
                *((uint32_t*)retAddr) = System::gfxDevice->height;
                return true;
            }
            else if(String::strcmp(items[2].id, "framebuffer")) {
                *((uint32_t*)retAddr) = System::gfxDevice->framebufferPhys;
                return true;
            }
            else if(String::strcmp(items[2].id, "name")) {
                char* targ = (char*)retAddr;
                int len = String::strlen(System::gfxDevice->identifier);
                MemoryOperations::memcpy(targ, System::gfxDevice->identifier, len);
                targ[len] = '\0';
                return true;
            }
            else
                return false;
        }
        else if(String::strcmp(items[1].id, "processes")) {
            if(getSize) {
                *((int*)retAddr) = ProcessHelper::Processes.size();
                return true;
            }
            if(items[2].type != LIBCactusOS::SIPropertyIdentifier::Index || items[3].type != LIBCactusOS::SIPropertyIdentifier::String)
                return false; // Needs to be index for collection and next needs to be property id

            int index = items[2].index;
            if(String::strcmp(items[3].id, "pid")) {
                *((int*)retAddr) = ProcessHelper::Processes[index]->id;
                return true;
            }
            else if(String::strcmp(items[3].id, "userspace")) {
                *((bool*)retAddr) = ProcessHelper::Processes[index]->isUserspace;
                return true;
            }
            else if(String::strcmp(items[3].id, "state")) {
                *((int*)retAddr) = (int)ProcessHelper::Processes[index]->state;
                return true;
            }
            else if(String::strcmp(items[3].id, "membase")) {
                *((uint32_t*)retAddr) = ProcessHelper::Processes[index]->excecutable.memBase;
                return true;
            }
            else if(String::strcmp(items[3].id, "memsize")) {
                *((uint32_t*)retAddr) = ProcessHelper::Processes[index]->excecutable.memSize;
                return true;
            }
            else if(String::strcmp(items[3].id, "heap-start")) {
                *((uint32_t*)retAddr) = ProcessHelper::Processes[index]->heap.heapStart;
                return true;
            }
            else if(String::strcmp(items[3].id, "heap-end")) {
                *((uint32_t*)retAddr) = ProcessHelper::Processes[index]->heap.heapEnd;
                return true;
            }
            else if(String::strcmp(items[3].id, "filename")) {
                char* targ = (char*)retAddr;
                int len = String::strlen(ProcessHelper::Processes[index]->fileName);
                MemoryOperations::memcpy(targ, ProcessHelper::Processes[index]->fileName, len);
                targ[len] = '\0';
                return true;
            }
            else
                return false;
        }
        else if(String::strcmp(items[1].id, "memory")) {
            if(items[2].type != LIBCactusOS::SIPropertyIdentifier::String)
                return false; // Needs to be property id
            
            if(String::strcmp(items[2].id, "total")) {
                *((uint32_t*)retAddr) = PhysicalMemoryManager::AmountOfMemory();
                return true;
            }
            else if(String::strcmp(items[2].id, "used")) {
                *((uint32_t*)retAddr) = PhysicalMemoryManager::UsedBlocks() * PAGE_SIZE;
                return true;
            }
            else if(String::strcmp(items[2].id, "free")) {
                *((uint32_t*)retAddr) = PhysicalMemoryManager::FreeBlocks() * PAGE_SIZE;
                return true;
            }
            else
                return false;
        }
        else
            return false; // Unknown identifier
    }
    else
        return false; // Unknown category
}