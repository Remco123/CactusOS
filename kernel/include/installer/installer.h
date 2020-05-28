#ifndef __CACTUSOS__INSTALLER__INSTALLER_H
#define __CACTUSOS__INSTALLER__INSTALLER_H

#include <system/disks/partitionmanager.h>
#include <system/vfs/fat.h>

namespace CactusOS
{
    // Class responsible for installing CactusOS on a hard drive
    class Installer
    {
    public:
        static void SetupError();
        static void Run();
        static char GetKey();

        // Installer Steps
        static void ShowWelcomeMessage();
        static void ShowWarningMessage();
        static void ShowDiskSelection();
        static void ShowDiskEraseMenu();
        static void ShowInstallScreen();
        static void ShowSystemCopyScreen(system::FAT* fatFS);
        static void CreateFatPartition(system::PartitionTableEntry* pEntry);
    };

    #define KEY_ENTER       0x1C
    #define KEY_ARROW_UP    0x48
    #define KEY_ARROW_DOWN  0x50
    #define KEY_LEFT_ARROW  0x33
    #define KEY_RIGHT_ARROW 0x34
    #define KEY_MINUS       0x0C
    #define KEY_PLUS        0x0D
    #define KEY_A           0x1E
}

#endif