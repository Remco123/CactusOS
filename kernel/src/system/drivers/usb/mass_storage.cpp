#include <system/drivers/usb/mass_storage.h>
#include <system/drivers/usb/usbdefs.h>
#include <system/log.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

//Create new driver for a MSD
USBMassStorageDriver::USBMassStorageDriver(USBDevice* dev)
: USBDriver(dev, "USB Mass Storage"), Disk(0, 0, USBDisk, 0)
{ }

//Called when mass storage device is plugged into system
bool USBMassStorageDriver::Initialize()
{
    for(USBEndpoint* ep : this->device->endpoints) {
        if(ep->type == EndpointType::Bulk) { // Bulk Endpoint
            if(ep->dir == EndpointDirection::In) //In
                this->bulkInEP = ep->endpointNumber;
            else //Out
                this->bulkOutEP = ep->endpointNumber;
        }
    }
    this->maxLUN = this->device->controller->GetMaxLuns(this->device);
    
    Log(Info, "MSD: Bulk in=%d Bulk Out=%d LUNS=%d", this->bulkInEP, this->bulkOutEP, this->maxLUN + 1);

    ////////////
    // Inquiry
    ////////////
    {
        CommandBlockWrapper* sendBuf = SCSIPrepareCommandBlock(SCSI_INQUIRY, sizeof(InquiryReturnBlock));
        InquiryReturnBlock retBuf;
        if(SCSIRequestIn(sendBuf, (uint8_t*)&retBuf, sizeof(InquiryReturnBlock))) {
            char vendorInfo[8 + 1]; vendorInfo[8] = '\0'; MemoryOperations::memcpy(vendorInfo, retBuf.vendorInformation, 8);
            char productInfo[16 + 1]; productInfo[16] = '\0'; MemoryOperations::memcpy(productInfo, retBuf.productIdentification, 16);
            char revision[4 + 1]; revision[4] = '\0'; MemoryOperations::memcpy(revision, retBuf.productRevisionLevel, 4);

            Log(Info, "MSD Vendor: %s Product: %s Revision: %s", vendorInfo, productInfo, revision);

            //////////
            // Create Identifier
            //////////
            int strLen = 16;
            while(productInfo[strLen - 1] == ' ' && strLen > 1)
                strLen--;
            this->identifier = new char[strLen + 1];
            MemoryOperations::memcpy(this->identifier, productInfo, strLen);
            this->identifier[strLen] = '\0';
        }
        else {
            delete sendBuf;
            return false;
        }
        delete sendBuf;
    }

    ////////////
    // Read Format Capacities
    ////////////
    /*
    for(int i = 0; i < 3; i++) //Try 3 times since first time will probably not work
    {
        CommandBlockWrapper* sendBuf = SCSIPrepareCommandBlock(SCSI_READ_FORMAT_CAPACITIES, 0xFC);
        uint8_t retBuf[0xFC];
        if(SCSIRequestIn(sendBuf, retBuf, 0xFC)) {
            CapacityDescriptor* c = (CapacityDescriptor*)(retBuf + sizeof(CapacityListHeader));
            MemoryOperations::memcpy(&this->formatCapacity, c, sizeof(CapacityDescriptor));

            delete sendBuf;
            break;
        }
        else
        {
            ////////////
            // Request Sense
            ////////////
            CommandBlockWrapper* sendBuf = SCSIPrepareCommandBlock(SCSI_REQUEST_SENSE, sizeof(RequestSenseBlock));
            RequestSenseBlock retBuf;
            SCSIRequestIn(sendBuf, (uint8_t*)&retBuf, sizeof(RequestSenseBlock));
            i = i + 1;
            i = i - 1;
        }
        delete sendBuf;
    }
    */
    
    ////////////
    // Read Capacity (10)
    ////////////
    CommandBlockWrapper* sendBuf = SCSIPrepareCommandBlock(SCSI_READ_CAPACITY_10, sizeof(Capacity10Block));
    Capacity10Block retBuf;
    if(SCSIRequestIn(sendBuf, (uint8_t*)&retBuf, sizeof(Capacity10Block))) {
        asm("bswap %0" : "+r"(retBuf.logicalBlockAddress));
        asm("bswap %0" : "+r"(retBuf.blockLength));
        if(retBuf.logicalBlockAddress == 0xFFFFFFFF) //We need to use Read Capacity 16
        {
            ////////////
            // Read Capacity (16)
            ////////////
            {
                CommandBlockWrapper* sendBuf = SCSIPrepareCommandBlock(SCSI_READ_CAPACITY_16, 8);
                Capacity16Block retBuf;
                if(SCSIRequestIn(sendBuf, (uint8_t*)&retBuf, 8)) {
                    asm("bswap %0" : "+r"(retBuf.logicalBlockAddress));
                    asm("bswap %0" : "+r"(retBuf.blockLength));

                    this->numBlocks = retBuf.logicalBlockAddress + 1;
                    this->blockSize = retBuf.blockLength;
                    this->use16Base = true;
                }
                else {
                    delete sendBuf;
                    return false;
                }
                delete sendBuf;
            }
        }
        else
        {
            this->numBlocks = retBuf.logicalBlockAddress + 1;
            this->blockSize = retBuf.blockLength;
        }
    }
    else {
        delete sendBuf;
        return false;
    }
    delete sendBuf;
    Log(Info, "MSD Blocks=%d BlockSize=%d Size=%d Mb", this->numBlocks, this->blockSize, ((uint64_t)this->numBlocks * (uint64_t)this->blockSize) / 1024 / 1024);
    this->size = this->numBlocks * this->blockSize;

    System::diskManager->AddDisk(this);
    
    return true;
}
//Prepare Command Block for a specific request
CommandBlockWrapper* USBMassStorageDriver::SCSIPrepareCommandBlock(uint8_t command, int length, uint64_t lba, int sectors)
{
    CommandBlockWrapper* cmd = new CommandBlockWrapper();
    MemoryOperations::memset(cmd, 0, sizeof(CommandBlockWrapper));

    //Default parameters for each request
    cmd->signature = CBW_SIGNATURE;
    cmd->tag = 0xBEEF;
    cmd->transferLen = length;
    cmd->command[0] = command;

    switch(command)
    {
        case SCSI_TEST_UNIT_READY:
            cmd->flags = 0x0;
            cmd->cmdLen = 0x6;
            break;
        case SCSI_REQUEST_SENSE:
            cmd->flags = 0x80;
            cmd->cmdLen = 0x6;
            cmd->command[4] = 18;
            break;
        case SCSI_INQUIRY:
            cmd->flags = 0x80;
            cmd->cmdLen = 0x6;
            cmd->command[4] = 36;
            break;
        case SCSI_READ_FORMAT_CAPACITIES:
            cmd->flags = 0x80;
            cmd->cmdLen = 10;
            cmd->command[8] = 0xFC;
            break;
        case SCSI_READ_CAPACITY_10:
            cmd->flags = 0x80;
            cmd->cmdLen = 10;
            break;
        case SCSI_READ_CAPACITY_16:
            cmd->flags = 0x80;
            cmd->cmdLen = 16;
            cmd->command[13] = 32;
            break;
        case SCSI_READ_10:
            cmd->flags = 0x80;
            cmd->cmdLen = 10;
            cmd->command[2] = (lba >> 24) & 0xFF;
            cmd->command[3] = (lba >> 16) & 0xFF;
            cmd->command[4] = (lba >> 8) & 0xFF;
            cmd->command[5] = (lba >> 0) & 0xFF;
            cmd->command[7] = (sectors >> 8) & 0xFF;
            cmd->command[8] = (sectors >> 0) & 0xFF;
            break;
        case SCSI_WRITE_10:
            cmd->flags = 0x0;
            cmd->cmdLen = 10;
            cmd->command[2] = (lba >> 24) & 0xFF;
            cmd->command[3] = (lba >> 16) & 0xFF;
            cmd->command[4] = (lba >> 8) & 0xFF;
            cmd->command[5] = (lba >> 0) & 0xFF;
            cmd->command[7] = (sectors >> 8) & 0xFF;
            cmd->command[8] = (sectors >> 0) & 0xFF;
            break;
        case SCSI_READ_16:
            cmd->flags = 0x80;
            cmd->cmdLen = 16;
            cmd->command[2] = (lba >> 56) & 0xFF;
            cmd->command[3] = (lba >> 48) & 0xFF;
            cmd->command[4] = (lba >> 40) & 0xFF;
            cmd->command[5] = (lba >> 32) & 0xFF;
            cmd->command[6] = (lba >> 24) & 0xFF;
            cmd->command[7] = (lba >> 16) & 0xFF;
            cmd->command[8] = (lba >> 8) & 0xFF;
            cmd->command[9] = (lba >> 0) & 0xFF;
            cmd->command[10] = (sectors >> 24) & 0xFF;
            cmd->command[11] = (sectors >> 16) & 0xFF;
            cmd->command[12] = (sectors >> 8) & 0xFF;
            cmd->command[13] = (sectors >> 0) & 0xFF;
            break;
        case SCSI_WRITE_16:
            cmd->flags = 0x0;
            cmd->cmdLen = 16;
            cmd->command[2] = (lba >> 56) & 0xFF;
            cmd->command[3] = (lba >> 48) & 0xFF;
            cmd->command[4] = (lba >> 40) & 0xFF;
            cmd->command[5] = (lba >> 32) & 0xFF;
            cmd->command[6] = (lba >> 24) & 0xFF;
            cmd->command[7] = (lba >> 16) & 0xFF;
            cmd->command[8] = (lba >> 8) & 0xFF;
            cmd->command[9] = (lba >> 0) & 0xFF;
            cmd->command[10] = (sectors >> 24) & 0xFF;
            cmd->command[11] = (sectors >> 16) & 0xFF;
            cmd->command[12] = (sectors >> 8) & 0xFF;
            cmd->command[13] = (sectors >> 0) & 0xFF;
            break;         
        default:
            Log(Error, "Unkown Command %b", command);
            break;
    }
    return cmd;
}
//Send request to device and put response in returnData
bool USBMassStorageDriver::SCSIRequestIn(CommandBlockWrapper* request, uint8_t* returnData, int returnLen)
{
    if(this->device->controller->BulkOut(this->device, request, sizeof(CommandBlockWrapper), this->bulkOutEP))
    {
        if(this->device->controller->BulkIn(this->device, returnData, returnLen, this->bulkInEP))
        {
            CommandStatusWrapper status;
            if(this->device->controller->BulkIn(this->device, &status, sizeof(CommandStatusWrapper), this->bulkInEP))
            {
                if((status.signature == CSW_SIGNATURE) && (status.status == 0) && (status.tag == request->tag))
                    return true;
                else
                    Log(Error, "Something wrong with Command Status Wrapper");
            }
            else
                Log(Error, "Error reading Command Status Wrapper from bulk in endpoint");
        }
        else
            Log(Error, "Error reading data back after command %b from bulk in endpoint, len=%d", request->command[0], returnLen);
    }
    else
        Log(Error, "Error Sending command %b to bulk out endpoint", request->command[0]);
    
    return false;
}
//Send request to device and then the specified data
bool USBMassStorageDriver::SCSIRequestOut(CommandBlockWrapper* request, uint8_t* sendData, int sendLen)
{
    if(this->device->controller->BulkOut(this->device, request, sizeof(CommandBlockWrapper), this->bulkOutEP))
    {
        if(this->device->controller->BulkOut(this->device, sendData, sendLen, this->bulkInEP))
        {
            CommandStatusWrapper status;
            if(this->device->controller->BulkIn(this->device, &status, sizeof(CommandStatusWrapper), this->bulkInEP))
            {
                if((status.signature == CSW_SIGNATURE) && (status.status == 0) && (status.tag == request->tag))
                    return true;
                else
                    Log(Error, "Something wrong with Command Status Wrapper");
            }
            else
                Log(Error, "Error reading Command Status Wrapper from bulk in endpoint");
        }
        else
            Log(Error, "Error sending data after command %b to bulk out endpoint, len=%d", request->command[0], sendLen);
    }
    else
        Log(Error, "Error Sending command %b to bulk out endpoint", request->command[0]);
    
    return false;
}

//Called when mass storage device is unplugged from system
void USBMassStorageDriver::DeInitialize()
{
    System::diskManager->RemoveDisk(this);
}

//Read Sector from mass storage device
char USBMassStorageDriver::ReadSector(common::uint32_t lba, common::uint8_t* buf)
{
    CommandBlockWrapper* sendBuf = SCSIPrepareCommandBlock(this->use16Base ? SCSI_READ_16 : SCSI_READ_10, this->blockSize, lba, 1);
    if(SCSIRequestIn(sendBuf, buf, this->blockSize)) {
        delete sendBuf;
        return 0; //Command Succes
    }
    delete sendBuf;
    
    //Error with request, so we try to receive error code
    sendBuf = SCSIPrepareCommandBlock(SCSI_REQUEST_SENSE, sizeof(RequestSenseBlock));
    RequestSenseBlock status;
    if(SCSIRequestIn(sendBuf, (uint8_t*)&status, sizeof(RequestSenseBlock)))
    {
        delete sendBuf;
        return status.errorCode;
    }

    delete sendBuf;
    Log(Error, "Error requesting sense after read sector %x fail", lba);
    return -1;
}

//Write Sector to mass storage device
char USBMassStorageDriver::WriteSector(common::uint32_t lba, common::uint8_t* buf)
{
    CommandBlockWrapper* sendBuf = SCSIPrepareCommandBlock(this->use16Base ? SCSI_WRITE_16 : SCSI_WRITE_10, this->blockSize, lba, 1);
    if(SCSIRequestOut(sendBuf, buf, this->blockSize)) {
        delete sendBuf;
        return 0; //Command Succes
    }
    delete sendBuf;
    
    //Error with request, so we try to receive error code
    sendBuf = SCSIPrepareCommandBlock(SCSI_REQUEST_SENSE, sizeof(RequestSenseBlock));
    RequestSenseBlock status;
    if(SCSIRequestIn(sendBuf, (uint8_t*)&status, sizeof(RequestSenseBlock)))
    {
        delete sendBuf;
        return status.errorCode;
    }

    delete sendBuf;
    Log(Error, "Error requesting sense after write sector %x fail", lba);
    return -1;
}