#include <system/debugger.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::core;

// Filename of the kernel symbol file
const char* symbolFileName = "B:\\debug.sym";

// Static initializer
List<KernelSymbol_t> KernelDebugger::symbolTable = List<KernelSymbol_t>();

void KernelDebugger::Initialize()
{
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