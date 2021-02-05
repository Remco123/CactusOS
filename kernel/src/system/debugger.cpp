#include <system/debugger.h>
#include <system/system.h>
#include <system/memory/deviceheap.h>
#include <system/listings/systeminfo.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::core;

// Filename of the kernel symbol file
const char* symbolFileName = "B:\\debug.sym";

// Static initializers
List<KernelSymbol_t> KernelDebugger::symbolTable = List<KernelSymbol_t>();
char KernelDebugger::messageBuffer[200] = {};
uint32_t KernelDebugger::pageAccessAddress = 0;

void KernelDebugger::Initialize()
{
    MemoryOperations::memset(KernelDebugger::messageBuffer, 0, sizeof(KernelDebugger::messageBuffer));

    // Check if file even exists
    if(System::vfs->FileExists(symbolFileName) == false) {
        Log(Error, "Could not initialize debugger %s does not exist!", symbolFileName);
        return;
    }

    // Get length and read file
    uint32_t fileLen = System::vfs->GetFileSize(symbolFileName);
    if(fileLen == (uint32_t)-1)
        return; // Error while getting size
    
    // Create buffer
    uint8_t* fileBuffer = new uint8_t[fileLen];
    if(System::vfs->ReadFile(symbolFileName, fileBuffer) != 0) {
        Log(Error, "Could not read file: %s!", symbolFileName);

        delete fileBuffer;
        return;
    }

    // Loop though all data and parse items
    uint32_t dataOffset = 0;
    while(dataOffset < fileLen)
    {
        // Turn data to string
        char* strPtr = (char*)(fileBuffer + dataOffset);

        // Create item
        KernelSymbol_t item;

        // Get address
        strPtr[8] = '\0'; // Replace space with terminator temporarily
        item.address = Convert::HexToInt(strPtr);
        strPtr[8] = ' '; // Replace it back

        // Get Type
        item.type = strPtr[9];

        // Copy name
        int len = String::IndexOf(strPtr + 10, '\n');
        item.name = new char[len];
        MemoryOperations::memcpy(item.name, strPtr + 11, len - 1);
        item.name[len-1] = '\0';

        dataOffset += len + 11;
        KernelDebugger::symbolTable.push_back(item);
    }
    delete fileBuffer;

    Log(Info, "Debugger initialized with %d symbols", KernelDebugger::symbolTable.size());

#if ENABLE_ADV_DEBUG
    KernelDebugger::pageAccessAddress = (uint32_t)KernelHeap::allignedMalloc(PAGE_SIZE, PAGE_SIZE);

    // Send message to external debugger that we are initalized and ready to receive commmands
    if(Serialport::Initialized && !System::gdbEnabled) {
        Serialport::WriteStr("$DebugReady\n");

        // Send system general info
        Serialport::WriteStr("$DebugSysSummary|");
        Serialport::WriteStr(__DATE__ "  " __TIME__ "|");

        // Info about system
        Serialport::WriteStr(SystemInfoManager::system.manufacturer);
        Serialport::WriteStr("|");
        Serialport::WriteStr(SystemInfoManager::system.product);
        Serialport::WriteStr("|");
        Serialport::WriteStr(SystemInfoManager::system.version);
        Serialport::WriteStr("|");

        // Info about bios
        Serialport::WriteStr(SystemInfoManager::bios.vendor);
        Serialport::WriteStr("|");
        Serialport::WriteStr(SystemInfoManager::bios.version);
        Serialport::WriteStr("|");

        // Info about memory
        Serialport::WriteStr(Convert::IntToString32(PhysicalMemoryManager::AmountOfMemory()));
        Serialport::WriteStr("\n"); // Terminate info

        KernelDebugger::PrintPageTables();
    }
#endif
}
const char* KernelDebugger::FindSymbol(uint32_t address, uint32_t* offset)
{    
	KernelSymbol_t prevItem = KernelDebugger::symbolTable[0];
	for (int i = 0; i < KernelDebugger::symbolTable.size(); i++)
	{
        // Check if address is between this entry or the last one
        // Then the address belongs to this function
		if (address >= prevItem.address && address <= KernelDebugger::symbolTable[i].address)
		{
			*offset = address - prevItem.address;
			return prevItem.name;
		}
		prevItem = KernelDebugger::symbolTable[i];
	}
	return 0;
}
void KernelDebugger::Stacktrace(CPUState* esp)
{
    if(KernelDebugger::symbolTable.size() == 0) {
        Log(Error, "Debugger symbols not loaded!");
        return;
    }

    StackFrame_t* frame = (StackFrame_t*)esp->EBP;
	uint32_t page = (uint32_t)frame & 0xFFFFF000;
    uint32_t offset = 0;

	const char* name = FindSymbol(esp->EIP, &offset);
    Log(Info, "%x [%x] %s", offset, esp->EIP, name ? name : "[Symbol not found]");
	while(frame && ((uint32_t)frame & 0xFFFFF000) == page)
	{
		name = FindSymbol(frame->addr, &offset);
        Log(Info, "%x [%x] %s", offset, frame->addr, name ? name : "[Symbol not found]");
		frame = frame->next;
	}
}
void KernelDebugger::Update()
{
    static int index = 0;

    while(Serialport::SerialReceiveReady()) {
        char c = Serialport::Read();
        if(c != '\n') {
            messageBuffer[index++] = c;
        }
        else {
            // Terminate string
            messageBuffer[index] = '\0';

            // Prevent a task-switch from happening while excecuting a debug command
            System::scheduler->Enabled = false;
            
            // Handle command
            KernelDebugger::HandleDebugCommand(index);

            // And enable scheduler again
            System::scheduler->Enabled = true;

            index = 0;
        }
    }
}
void KernelDebugger::HandleDebugCommand(int size)
{
    //Log(Info, "%s", messageBuffer);
    if (String::strncmp(messageBuffer, "ReqDebugUpdate", 14)) {
        // Update from kernel
        KernelDebugger::SendUpdateToHost();
    }
    else if(String::strncmp(messageBuffer, "xp ", 3)) {
        // Physical memory dump
        List<char*> args = String::Split(messageBuffer, ' ');

        uint32_t address = args[1][1] == 'x' ? Convert::HexToInt(args[1] + 2) : Convert::StringToInt(args[1]);
        uint32_t size = args[2][1] == 'x' ? Convert::HexToInt(args[2] + 2) : Convert::StringToInt(args[2]);

        KernelDebugger::PrintMemoryDump(address, size, false);

        for(char* c : args)
            delete c;
    }
    else if(String::strncmp(messageBuffer, "x ", 2)) {
        // Virtual memory dump
        List<char*> args = String::Split(messageBuffer, ' ');

        uint32_t address = args[1][1] == 'x' ? Convert::HexToInt(args[1] + 2) : Convert::StringToInt(args[1]);
        uint32_t size = args[2][1] == 'x' ? Convert::HexToInt(args[2] + 2) : Convert::StringToInt(args[2]);

        KernelDebugger::PrintMemoryDump(address, size, true);

        for(char* c : args)
            delete c;
    }
    else if(String::strncmp(messageBuffer, "pagedump", 8)) {
        // Page table dump
        KernelDebugger::PrintPageTables();
    }
    else {
        Log(Error, "Unknown debug command %s", messageBuffer);
    }
}
void KernelDebugger::SendUpdateToHost()
{
    // Send update about host to debugger
    Serialport::WriteStr("$DebugUpdate|");

    // Used physical memory
    Serialport::WriteStr(Convert::IntToString32(PhysicalMemoryManager::UsedBlocks() * PAGE_SIZE));
    Serialport::WriteStr("|");

    // Used kernel memory
    Serialport::WriteStr(Convert::IntToString32(KernelHeap::UsedMemory()));
    Serialport::WriteStr("|");

    // Idle process active time
    Serialport::WriteStr(Convert::IntToString32(System::statistics.idleProcActive));
    Serialport::WriteStr("|");

    // Disk read and writes
    Serialport::WriteStr(Convert::IntToString32(System::statistics.diskReadOp));
    Serialport::WriteStr("|");
    Serialport::WriteStr(Convert::IntToString32(System::statistics.diskWriteOp));

    Serialport::WriteStr("\n");
}
void KernelDebugger::PrintMemoryDump(uint32_t address, uint32_t size, bool virtMemory)
{
    //Log(Info, "KernelDebugger::PrintMemoryDump(%x, %x, %d)", address, size, virtMemory);

    const uint8_t maxWidth = 20;

    BootConsole::WriteLine("------------ Memory Dump ----------");
    if(virtMemory)
    {
        // Print a byte dump of a virtual region of memory
        uint8_t width = 0;
        for(uint32_t x = address; x < address + size; x++) {
            BootConsole::Write(Convert::IntToHexString(*(uint8_t*)x));
            BootConsole::Write(" ");
            
            if (width++ > maxWidth) {
                width = 0;
                BootConsole::WriteLine();
            }
        }
    }
    else
    {   
        // We have section at the start we need to print
        if(address % PAGE_SIZE != 0) {
            uint32_t start = KernelDebugger::pageAccessAddress + (address % PAGE_SIZE);
            uint16_t firstBlockSize = PAGE_SIZE - (address % PAGE_SIZE);
            if(firstBlockSize > size)
                firstBlockSize = size;

            // We are processing this block
            size -= firstBlockSize;

            // Move address to paging boundary
            address = pageRoundDown(address);

            // Make sure we can access this physical block of memory
            VirtualMemoryManager::mapVirtualToPhysical((void*)address, (void*)KernelDebugger::pageAccessAddress, true, false);

            uint8_t width = 0;
            for(uint32_t item = 0; item < firstBlockSize; item++) {
                BootConsole::Write(Convert::IntToHexString(*(uint8_t*)(start + item)));
                BootConsole::Write(" ");
                
                if (width++ > maxWidth) {
                    width = 0;
                    BootConsole::WriteLine();
                }
            }
        }

        // Allocate a piece of memory for accessing the physical memory
        for(uint32_t x = address; x < address + size; x += PAGE_SIZE)
        {
            // Make sure we can access this physical block of memory
            VirtualMemoryManager::mapVirtualToPhysical((void*)x, (void*)KernelDebugger::pageAccessAddress, true, false);

            uint16_t blockSize = (x - (address + size)) > PAGE_SIZE ? PAGE_SIZE : (x - (address + size));

            uint8_t width = 0;
            for(uint32_t item = 0; item < blockSize; item++) {
                BootConsole::Write(Convert::IntToHexString((*(uint8_t*)(KernelDebugger::pageAccessAddress + item))));
                BootConsole::Write(" ");
                
                if (width++ > maxWidth) {
                    width = 0;
                    BootConsole::WriteLine();
                }
            }
        }
    }
    BootConsole::WriteLine();
    BootConsole::WriteLine("-----------------------------------");
}
void KernelDebugger::PrintPageTables()
{
    char* flagBuf = "-----";

    BootConsole::WriteLine("------------ Paging Dump ----------");

    PageDirectory* pageDir = (PageDirectory*)PAGE_DIRECTORY_ADDRESS;

    // Loop through all page directories and print their contents
    for(uint16_t pageDirIndex = 0; pageDirIndex < 1024; pageDirIndex++) {
        // Get entry from page directory
        PageDirectoryEntry pdEntry = pageDir->entries[pageDirIndex];
        
        if(pdEntry.pageSize == FOUR_MB) {
            if(pdEntry.present) {
                // Start
                BootConsole::Write("0x");
                BootConsole::Write(Convert::IntToHexString(pageDirIndex * 4_MB));
                BootConsole::Write("  0x");
                BootConsole::Write(Convert::IntToHexString(pdEntry.frame * 4_MB));
                BootConsole::Write("  0x");
                BootConsole::Write(Convert::IntToHexString(pdEntry.frame * 4_MB + 4_MB));
                BootConsole::Write(" ");

                flagBuf[0] = 'R';
                if(pdEntry.readWrite)
                    flagBuf[1] = 'W';
                if(pdEntry.isUser)
                    flagBuf[2] = 'U';
                if(pdEntry.writeThrough)
                    flagBuf[3] = 'W';
                if(pdEntry.canCache)
                    flagBuf[4] = 'C';
                
                BootConsole::WriteLine(flagBuf);
            }
        }
        else {
            if(pdEntry.present)
                BootConsole::WriteLine("----------");
        }
    }

    BootConsole::WriteLine("-----------------------------------");
}