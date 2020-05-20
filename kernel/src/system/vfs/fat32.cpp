#include <system/vfs/fat32.h>

#include <common/print.h>
#include <system/log.h>
#include <system/system.h>

using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

FAT32::FAT32(Disk* disk, uint32_t start, uint32_t size)
: VirtualFileSystem(disk, start, size) 
{
    this->Name = "FAT32 Filesystem";
}

bool FAT32::Initialize()
{
    BootConsole::WriteLine();
    BootConsole::WriteLine("Initializing FAT Filesystem");
    
    char* str = "0:";
    str[0] = Convert::IntToString(System::diskManager->allDisks.IndexOf(this->disk))[0];
    
    if(f_mount(&this->baseFS, str, 0) == FR_OK) {
        uint8_t oldColor = BootConsole::ForegroundColor;
        BootConsole::ForegroundColor = VGA_COLOR_GREEN;
        BootConsole::Write("FAT32 Filesystem Intialized");
        BootConsole::ForegroundColor = oldColor;
        return true;
    }
    return false;
}

FRESULT scan_files (const char* path, List<char*>** target)
{
    FRESULT res;
    DIR dir;
    static FILINFO fno;

    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            
            int i = String::strlen(fno.fname);
            char* str = new char[i + 1];
            str[i] = '\0';

            MemoryOperations::memcpy(str, fno.fname, i);
            (*target)->push_back(str);
        }
        f_closedir(&dir);
    }

    return res;
}

List<char*>* FAT32::DirectoryList(const char* path)
{ 
    List<char*>* ret = new List<char*>();
    FRESULT res = scan_files(path, &ret);
    if(res == FR_OK)
        return ret;
    
    delete ret;
    return 0;
}

uint32_t FAT32::GetFileSize(const char* path)
{
    FIL fp;
    if(f_open(&fp, path, FA_OPEN_EXISTING) != FR_OK)
        return (uint32_t)-1;
    
    uint32_t ret = f_size(&fp);
    f_close(&fp);

    return ret;
}
int FAT32::ReadFile(const char* path, uint8_t* buffer, uint32_t offset, uint32_t len)
{ 
    if(len == -1)
        len = GetFileSize(path);
    
    FIL fp;
    if(f_open(&fp, path, FA_READ) != FR_OK)
        return (uint32_t)-1;
    
    UINT br;
    f_lseek(&fp, offset);
    f_read(&fp, buffer, len, &br);
    f_close(&fp);

    return 0;
}

int FAT32::WriteFile(const char* path, uint8_t* buffer, uint32_t len, bool create)
{
    FIL fp;
    if(f_open(&fp, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
        return (uint32_t)-1;
    
    f_write(&fp, buffer, len, 0);
    f_close(&fp);

    return 0;
}

int FAT32::CreateFile(const char* path)
{
    FIL fp;
    FRESULT ret;
    ret = f_open(&fp, path, FA_CREATE_NEW);
    f_close(&fp);

    return (ret != FR_OK);
}

int FAT32::CreateDirectory(const char* path)
{
    return (f_mkdir(path) == FR_OK);
}

bool FAT32::FileExists(const char* path)
{ 
    FILINFO info;
    FRESULT fr;

    fr = f_stat(path, &info);

    if(fr == FR_OK && !(info.fattrib & AM_DIR))
        return true;

    return false;
}
bool FAT32::DirectoryExists(const char* path)
{ 
    FILINFO info;
    FRESULT fr;

    fr = f_stat(path, &info);

    if(fr == FR_OK && (info.fattrib & AM_DIR))
        return true;

    return false;
}