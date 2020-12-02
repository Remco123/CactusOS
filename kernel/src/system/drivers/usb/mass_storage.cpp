#include <system/drivers/usb/mass_storage.h>
#include <system/drivers/usb/usbdefs.h>
#include <system/log.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

// Create new driver for a MSD
USBMassStorageDriver::USBMassStorageDriver(USBDevice* dev)
: USBDriver(dev, "USB Mass Storage"), Disk(0, 0, USBDisk, 0, 0, 0)
{ }

// Called when mass storage device is plugged into system
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
    if(this->maxLUN == 0xFF)
        this->maxLUN = 0; // Some devices return 0xFF while they mean 0
    
    Log(Info, "MSD: Bulk in=%d Bulk Out=%d LUNS=%d", this->bulkInEP, this->bulkOutEP, this->maxLUN + 1);

    ///////////////
    // Test Unit Ready
    ///////////////
    CommandBlockWrapper checkReadyCMD = SCSIPrepareCommandBlock(SCSI_TEST_UNIT_READY, 0);
    for(int i = 0; i < 3; i++) {
        if(!SCSIRequest(&checkReadyCMD, 0, 0)) {
            Log(Warning, "MSD Device not ready yet");
            System::pit->Sleep(100);
        }
        else {
            break;
        }
    }


    ///////////////
    // Inquiry
    ///////////////
    CommandBlockWrapper inquiryCMD = SCSIPrepareCommandBlock(SCSI_INQUIRY, sizeof(InquiryReturnBlock));
    InquiryReturnBlock inquiryRet;
    if(SCSIRequest(&inquiryCMD, (uint8_t*)&inquiryRet, sizeof(InquiryReturnBlock))) {
        char vendorInfo[8 + 1]; vendorInfo[8] = '\0'; MemoryOperations::memcpy(vendorInfo, inquiryRet.vendorInformation, 8);
        char productInfo[16 + 1]; productInfo[16] = '\0'; MemoryOperations::memcpy(productInfo, inquiryRet.productIdentification, 16);
        char revision[4 + 1]; revision[4] = '\0'; MemoryOperations::memcpy(revision, inquiryRet.productRevisionLevel, 4);

        Log(Info, "MSD Vendor: %s Product: %s Revision: %s", vendorInfo, productInfo, revision);

        // Check if it is a Direct Access Block Device or CD-ROM/DVD device
        if(inquiryRet.peripheralDeviceType != 0x00 && inquiryRet.peripheralDeviceType != 0x05)
            return false;
        
        // Check if LUN is connected to something
        if(inquiryRet.peripheralQualifier != 0)
            return false;

        // Response Data Format should be 0x01 or 0x02
        if(inquiryRet.responseDataFormat != 0x01 && inquiryRet.responseDataFormat != 0x02)
            return false;

        // Create Identifier
        int strLen = 16;
        while(productInfo[strLen - 1] == ' ' && strLen > 1)
            strLen--;
        this->identifier = new char[strLen + 1];
        MemoryOperations::memcpy(this->identifier, productInfo, strLen);
        this->identifier[strLen] = '\0';
    }
    else {
        return false;
    }

    ///////////////
    // Read Format Capacities
    // We try this command 3 times since it will stall at least once
    ///////////////

    /*
    CommandBlockWrapper formatCapCMD = SCSIPrepareCommandBlock(SCSI_READ_FORMAT_CAPACITIES, 0xFC);
    uint8_t formatCapRet[0xFC];
    MemoryOperations::memset(formatCapRet, 0, 0xFC);

    for(int i = 0; i < 3; i++) 
    {
        if(SCSIRequest(&formatCapCMD, formatCapRet, 0xFC)) {
            CapacityListHeader* header = (CapacityListHeader*)formatCapRet;
            if(header->listLength > 0) {
                CapacityDescriptor* capDesc = (CapacityDescriptor*)(formatCapRet + 4);
                Log(Info, "MSD Read Format Capacities results: %d * %d = %x", capDesc->numberOfBlocks, capDesc->blockLength, capDesc->blockLength * capDesc->numberOfBlocks);

                // We have all the info we need, exit the for loop
                break;
            }
        }
        Log(Warning, "MSD Read Format Capacities try %d failed", i+1);
    }
    
    if(this->numBlocks == 0 || this->blockSize == 0)
        Log(Warning, "MSD Device does not support READ_FORMAT_CAPACITIES command");
    */
   
    ///////////////
    // Read Capacity
    // Command might not be required since we already got the data from READ_FORMAT_CAPACITIES
    ///////////////
    CommandBlockWrapper readCapacityCMD = SCSIPrepareCommandBlock(SCSI_READ_CAPACITY_10, sizeof(Capacity10Block));
    Capacity10Block readCapacityRet;
    if(SCSIRequest(&readCapacityCMD, (uint8_t*)&readCapacityRet, sizeof(Capacity10Block)))
    {
        readCapacityRet.logicalBlockAddress = __builtin_bswap32(readCapacityRet.logicalBlockAddress);
        readCapacityRet.blockLength = __builtin_bswap32(readCapacityRet.blockLength);

        if(readCapacityRet.logicalBlockAddress == 0xFFFFFFFF) // We need to use Read Capacity 16
        {
            ////////////
            // Read Capacity (16)
            ////////////
            CommandBlockWrapper readCapacity16CMD = SCSIPrepareCommandBlock(SCSI_READ_CAPACITY_16, sizeof(Capacity16Block));
            Capacity16Block readCapacityRet16;
            if(SCSIRequest(&readCapacity16CMD, (uint8_t*)&readCapacityRet16, sizeof(Capacity16Block))) {
                readCapacityRet16.logicalBlockAddress = __builtin_bswap64(readCapacityRet16.logicalBlockAddress);
                readCapacityRet16.blockLength = __builtin_bswap64(readCapacityRet16.blockLength);

                this->numBlocks = readCapacityRet16.logicalBlockAddress - 1;
                this->blockSize = readCapacityRet16.blockLength;
                this->use16Base = true;
            }
        }
        else
        {
            this->numBlocks = readCapacityRet.logicalBlockAddress - 1;
            this->blockSize = readCapacityRet.blockLength;
        }
    }


    Log(Info, "MSD Blocks=%d BlockSize=%d Size=%d Mb", this->numBlocks, this->blockSize, ((uint64_t)this->numBlocks * (uint64_t)this->blockSize) / 1024 / 1024);
    this->size = this->numBlocks * this->blockSize;

    System::diskManager->AddDisk(this);
    
    return true;
}
// Prepare Command Block for a specific request
CommandBlockWrapper USBMassStorageDriver::SCSIPrepareCommandBlock(uint8_t command, int length, uint64_t lba, int sectors)
{
    CommandBlockWrapper cmd;
    MemoryOperations::memset(&cmd, 0, sizeof(CommandBlockWrapper));

    //Default parameters for each request
    cmd.signature = CBW_SIGNATURE;
    cmd.tag = 0xBEEF;
    cmd.transferLen = length;
    cmd.command[0] = command;

    switch(command)
    {
        case SCSI_TEST_UNIT_READY:
            cmd.flags = 0x0;
            cmd.cmdLen = 0x6;
            break;
        case SCSI_REQUEST_SENSE:
            cmd.flags = 0x80;
            cmd.cmdLen = 0x6;
            cmd.command[4] = 18;
            break;
        case SCSI_INQUIRY:
            cmd.flags = 0x80;
            cmd.cmdLen = 0x6;
            cmd.command[4] = 36;
            break;
        case SCSI_READ_FORMAT_CAPACITIES:
            cmd.flags = 0x80;
            cmd.cmdLen = 10;
            cmd.command[8] = 0xFC;
            break;
        case SCSI_READ_CAPACITY_10:
            cmd.flags = 0x80;
            cmd.cmdLen = 10;
            break;
        case SCSI_READ_CAPACITY_16:
            cmd.flags = 0x80;
            cmd.cmdLen = 16;
            cmd.command[13] = 32;
            break;
        case SCSI_READ_10:
            cmd.flags = 0x80;
            cmd.cmdLen = 10;
            cmd.command[2] = (lba >> 24) & 0xFF;
            cmd.command[3] = (lba >> 16) & 0xFF;
            cmd.command[4] = (lba >> 8) & 0xFF;
            cmd.command[5] = (lba >> 0) & 0xFF;
            cmd.command[7] = (sectors >> 8) & 0xFF;
            cmd.command[8] = (sectors >> 0) & 0xFF;
            break;
        case SCSI_WRITE_10:
            cmd.flags = 0x0;
            cmd.cmdLen = 10;
            cmd.command[2] = (lba >> 24) & 0xFF;
            cmd.command[3] = (lba >> 16) & 0xFF;
            cmd.command[4] = (lba >> 8) & 0xFF;
            cmd.command[5] = (lba >> 0) & 0xFF;
            cmd.command[7] = (sectors >> 8) & 0xFF;
            cmd.command[8] = (sectors >> 0) & 0xFF;
            break;
        case SCSI_READ_16:
            cmd.flags = 0x80;
            cmd.cmdLen = 16;
            cmd.command[2] = (lba >> 56) & 0xFF;
            cmd.command[3] = (lba >> 48) & 0xFF;
            cmd.command[4] = (lba >> 40) & 0xFF;
            cmd.command[5] = (lba >> 32) & 0xFF;
            cmd.command[6] = (lba >> 24) & 0xFF;
            cmd.command[7] = (lba >> 16) & 0xFF;
            cmd.command[8] = (lba >> 8) & 0xFF;
            cmd.command[9] = (lba >> 0) & 0xFF;
            cmd.command[10] = (sectors >> 24) & 0xFF;
            cmd.command[11] = (sectors >> 16) & 0xFF;
            cmd.command[12] = (sectors >> 8) & 0xFF;
            cmd.command[13] = (sectors >> 0) & 0xFF;
            break;
        case SCSI_WRITE_16:
            cmd.flags = 0x0;
            cmd.cmdLen = 16;
            cmd.command[2] = (lba >> 56) & 0xFF;
            cmd.command[3] = (lba >> 48) & 0xFF;
            cmd.command[4] = (lba >> 40) & 0xFF;
            cmd.command[5] = (lba >> 32) & 0xFF;
            cmd.command[6] = (lba >> 24) & 0xFF;
            cmd.command[7] = (lba >> 16) & 0xFF;
            cmd.command[8] = (lba >> 8) & 0xFF;
            cmd.command[9] = (lba >> 0) & 0xFF;
            cmd.command[10] = (sectors >> 24) & 0xFF;
            cmd.command[11] = (sectors >> 16) & 0xFF;
            cmd.command[12] = (sectors >> 8) & 0xFF;
            cmd.command[13] = (sectors >> 0) & 0xFF;
            break;         
        default:
            Log(Error, "Unkown Command %b", command);
            break;
    }
    return cmd;
}
// Perform a reset recovery process after stall
bool USBMassStorageDriver::ResetRecovery()
{
    // First send the MSD Reset Request
    if(!this->device->controller->ControlOut(this->device, 0, HOST_TO_DEV | REQ_TYPE_CLASS | RECPT_INTERFACE, BULK_ONLY_RESET)) {
        Log(Error, "MSD, Reset Failed!");
        return false;
    }

    // Then the Clear feature for the IN-Endpoint
    if(!this->device->controller->ControlOut(this->device, 0, HOST_TO_DEV | REQ_TYPE_STNDRD | RECPT_ENDPOINT, CLEAR_FEATURE, 0, 0, this->bulkInEP)) {
        Log(Error, "MSD, Clear feature (HALT) Failed for Bulk-In!");
        return false;
    }
    this->device->endpoints[this->bulkInEP-1]->SetToggle(0);

    // Then the Clear feature for the OUT-Endpoint
    if(!this->device->controller->ControlOut(this->device, 0, HOST_TO_DEV | REQ_TYPE_STNDRD | RECPT_ENDPOINT, CLEAR_FEATURE, 0, 0, this->bulkOutEP)) {
        Log(Error, "MSD, Clear feature (HALT) Failed for Bulk-Out!");
        return false;
    }
    this->device->endpoints[this->bulkOutEP-1]->SetToggle(0);

    return true;
}
// Perform a SCSI In or Out operation on device
bool USBMassStorageDriver::SCSIRequest(CommandBlockWrapper* request, uint8_t* dataPointer, int dataLength)
{
    // Send request to device
    if(!this->device->controller->BulkOut(this->device, request, sizeof(CommandBlockWrapper), this->bulkOutEP)) {
        Log(Error, "Error Sending command %b to bulk out endpoint", request->command[0]);
        
        // Clear HALT for the OUT-Endpoint
        if(!this->device->controller->ControlOut(this->device, 0, HOST_TO_DEV | REQ_TYPE_STNDRD | RECPT_ENDPOINT, CLEAR_FEATURE, 0, 0, this->bulkOutEP)) {
            Log(Error, "MSD, Clear feature (HALT) Failed for Bulk-Out!");
            return false;
        }
        this->device->endpoints[this->bulkOutEP-1]->SetToggle(0);
    }

    // If this is a data command, recieve the data
    if(dataLength > 0) {
        if(request->flags == 0x80) { // In Transfer
            if(!this->device->controller->BulkIn(this->device, dataPointer, dataLength, this->bulkInEP)) {
                Log(Error, "Error receiving data after command %b from bulk endpoint, len=%d", request->command[0], dataLength);
                
                // Clear HALT feature for the IN-Endpoint
                if(!this->device->controller->ControlOut(this->device, 0, HOST_TO_DEV | REQ_TYPE_STNDRD | RECPT_ENDPOINT, CLEAR_FEATURE, 0, 0, this->bulkInEP)) {
                    Log(Error, "MSD, Clear feature (HALT) Failed for Bulk-In!");
                    return false;
                }
                this->device->endpoints[this->bulkInEP-1]->SetToggle(0);
            }
        }
        else { // Out Transfer
            if(!this->device->controller->BulkOut(this->device, dataPointer, dataLength, this->bulkOutEP)) {
                Log(Error, "Error sending data after command %b to bulk endpoint, len=%d", request->command[0], dataLength);
                
                // Clear HALT feature for the OUT-Endpoint
                if(!this->device->controller->ControlOut(this->device, 0, HOST_TO_DEV | REQ_TYPE_STNDRD | RECPT_ENDPOINT, CLEAR_FEATURE, 0, 0, this->bulkOutEP)) {
                    Log(Error, "MSD, Clear feature (HALT) Failed for Bulk-Out!");
                    return false;
                }
                this->device->endpoints[this->bulkOutEP-1]->SetToggle(0);                
            }
        }
    }

    // Receive status descriptor from device.
    // Also when receiving data failed this is required
    CommandStatusWrapper status;
    MemoryOperations::memset(&status, 0, sizeof(CommandStatusWrapper));
    if(!this->device->controller->BulkIn(this->device, &status, sizeof(CommandStatusWrapper), this->bulkInEP)) {
        Log(Error, "Error reading Command Status Wrapper from bulk in endpoint");

        // Clear HALT feature for the IN-Endpoint
        if(!this->device->controller->ControlOut(this->device, 0, HOST_TO_DEV | REQ_TYPE_STNDRD | RECPT_ENDPOINT, CLEAR_FEATURE, 0, 0, this->bulkInEP)) {
            Log(Error, "MSD, Clear feature (HALT) Failed for Bulk-In!");
            return false;
        }
        this->device->endpoints[this->bulkInEP-1]->SetToggle(0);        
    }

    if(status.signature != CSW_SIGNATURE) {
        this->ResetRecovery();
        return false;
    }

    if((status.status == 1) && (request->command[0] != SCSI_REQUEST_SENSE)) {
        // Command did not succeed so Request the Sense data
        CommandBlockWrapper requestSenseCMD = SCSIPrepareCommandBlock(SCSI_REQUEST_SENSE, sizeof(RequestSenseBlock));
        RequestSenseBlock requestSenseRet;

        if(SCSIRequest(&requestSenseCMD, (uint8_t*)&requestSenseRet, sizeof(RequestSenseBlock)))
            Log(Info, "MSD Request Sense after ReadCap: Error=%b Valid=%d Additional=%d Key=%d ASC=%b ASCQ=%b", requestSenseRet.errorCode, requestSenseRet.valid, requestSenseRet.additionalLength, requestSenseRet.senseKey, requestSenseRet.ASC, requestSenseRet.ASCQ);
        else
            Log(Error, "MSD Could not request sense after read capacity failure");

        return false;
    }

    if((status.status == 2) && (request->command[0] != SCSI_REQUEST_SENSE)) {
        this->ResetRecovery();
        return false;
    }

    if((status.status == 0) && (status.tag == request->tag))
        return true;
        
    Log(Error, "Something wrong with Command Status Wrapper, status = %d", status.status);
    return false;
}

// Called when mass storage device is unplugged from system
void USBMassStorageDriver::DeInitialize()
{
    System::diskManager->RemoveDisk(this);
}

// Read Sector from mass storage device
char USBMassStorageDriver::ReadSector(common::uint32_t lba, common::uint8_t* buf)
{
    this->readWriteLock.Lock();
    CommandBlockWrapper sendBuf = SCSIPrepareCommandBlock(this->use16Base ? SCSI_READ_16 : SCSI_READ_10, this->blockSize, lba, 1);
    if(SCSIRequest(&sendBuf, buf, this->blockSize)) {
        this->readWriteLock.Unlock();
        return 0; // Command Succes
    }

    Log(Error, "MSD Error reading sector %x", lba);

    this->readWriteLock.Unlock();
    return -1;
}

// Write Sector to mass storage device
char USBMassStorageDriver::WriteSector(common::uint32_t lba, common::uint8_t* buf)
{
    this->readWriteLock.Lock();
    CommandBlockWrapper sendBuf = SCSIPrepareCommandBlock(this->use16Base ? SCSI_WRITE_16 : SCSI_WRITE_10, this->blockSize, lba, 1);
    if(SCSIRequest(&sendBuf, buf, this->blockSize)) {
        this->readWriteLock.Unlock();
        return 0; // Command Succes
    }

    Log(Error, "MSD Error writing sector %x", lba);

    this->readWriteLock.Unlock();
    return -1;
}