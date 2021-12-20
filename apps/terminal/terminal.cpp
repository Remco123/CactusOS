#include <new.h>
#include <gui/gui.h>
#include <gui/widgets/window.h>
#include <gui/widgets/label.h>
#include <gui/directgui.h>
#include <string.h>
#include <log.h>
#include <proc.h>
#include <convert.h>
#include <ipc.h>
#include <vfs.h>
#include <time.h>
#include "terminalcontrol.h"

char* workingDir = "B:\\";
TerminalControl* termWindow = 0;

int ExecCommand(char* cmd);

void GUIThread()
{
    while(1)
    {
        static uint64_t ticks = 0;
        if(Time::Ticks() - ticks > 500) {
            termWindow->ToggleCursor();
            ticks = Time::Ticks();
        }

        GUI::DrawGUI();
        if(IPCAvailable())
            GUI::ProcessEvents();
        else
            Process::Yield();
    }
}
int main(int argc, char** argv)
{
    GUI::SetDefaultFont();
    
    Window* mainWindow = new Window(600, 400, GUI::Width/2 - 300, GUI::Width/2 - 200);
    mainWindow->titleString = "CactusOS Terminal";

    termWindow = new TerminalControl(mainWindow->width, mainWindow->height - 30);
    mainWindow->AddChild(termWindow);
    
    Process::CreateThread(GUIThread, false);
    while(GUI::HasItems())
    {
        mainWindow->titleString = workingDir;
        char* cmd = termWindow->ReadCommand(workingDir);
        int pid = ExecCommand(cmd);
        
        if(pid != 0)
        {
            Process::BindSTDIO(pid, Process::ID);
            Process::Unblock(pid);
            while(Process::Active(pid)) {
                if(Process::StdInAvailable() > 0)
                    termWindow->Write(Process::ReadStdIn());
            }

            //Read remaining data from process if available
            while(Process::StdInAvailable() > 0)
                termWindow->Write(Process::ReadStdIn());
        }
        
        delete cmd;
    }

    return 0;
}

// Execute command and return pid of created process if necessary
int ExecCommand(char* cmd)
{
    if(memcmp(cmd, "ls", 3) == 0)
    {
        List<VFSEntry> items = DirectoryListing(workingDir);
        for(VFSEntry item : items)
        {
            //termWindow->Write(item.isDir ? (char*)"DIR: " : (char*)"FILE: ");
            termWindow->Write(item.name); termWindow->Write('\n');
            //Print("[Terminal ls] File = %s Size = %d Date = %d:%d:%d Time = %d:%d:%d IsDir = %d\n", item.name, item.size, item.creationDate.year, item.creationDate.month, item.creationDate.day, item.creationTime.hour, item.creationTime.min, item.creationTime.sec, item.isDir);
        }
        return 0;
    }
    else if(memcmp(cmd, "cd", 3) == 0)
    {
        workingDir = "B:\\";
        return 0;
    }
    else if(memcmp(cmd, "cd ", 3) == 0) //cd with argument
    {
        int l = strlen(cmd+3);
        if(l <= 0)
            return 0;
        
        char* newWD = new char[l+1];
        memcpy(newWD, cmd + 3, l);
        newWD[l] = '\0';
        workingDir = newWD;
        
        return 0;
    }
    else if(memcmp(cmd, "proc", 5) == 0)
    {
        int procCount = SystemInfo::Properties["processes"].size();
        for(int i = 0; i < procCount; i++) {
            char* id = (char*)SystemInfo::Properties["processes"][i]["filename"];

            termWindow->Write("  ");
            termWindow->Write(id);
            termWindow->Write("\n  -> PID = ");
            termWindow->Write(Convert::IntToString((int)SystemInfo::Properties["processes"][i]["pid"]));
            termWindow->Write("\n  -> State = ");
            termWindow->Write(Convert::IntToString((int)SystemInfo::Properties["processes"][i]["state"]));
            termWindow->Write("\n  -> Heap = ");
            termWindow->Write(Convert::IntToString(((uint32_t)SystemInfo::Properties["processes"][i]["heap-end"] - (uint32_t)SystemInfo::Properties["processes"][i]["heap-start"]) / 1_KB));
            termWindow->Write(" Kb");
            termWindow->Write('\n');

            delete id;
        }

        return 0;
    }
    else if(memcmp(cmd, "shutdown", 8) == 0) {
        DoSyscall(SYSCALL_SHUTDOWN);
        return 0;
    }
    else if(memcmp(cmd, "reboot", 6) == 0) {
        DoSyscall(SYSCALL_REBOOT);
        return 0;
    }
    else if(memcmp(cmd, "lv", 2) == 0) {
        static char* diskTypes[] = {
                "HardDisk",
                "USBDisk",
                "Floppy",
                "CDROM"
        };

        int diskCount = SystemInfo::Properties["disks"].size();
        for(int i = 0; i < diskCount; i++) {
            char* id = (char*)SystemInfo::Properties["disks"][i]["identifier"];
            
            termWindow->Write(diskTypes[(int)SystemInfo::Properties["disks"][i]["type"]]); termWindow->Write(" ["); termWindow->Write(Convert::IntToString((uint32_t)SystemInfo::Properties["disks"][i]["size"] / 1_MB)); termWindow->Write(" MB] -> ");
            termWindow->Write(id); termWindow->Write('\n');

            delete id;
        }
        return 0;
    }
    else
    {
        // Combine working directory and cmd into one string
        char* comboCMD = str_Combine(workingDir, cmd);

        if(FileExists(comboCMD)) {
            int ret = Process::Run(comboCMD, true);
            if(ret == 0)
                termWindow->Write("File is not an executable\n");
            
            delete comboCMD;
            return ret;
        }
        else {
            termWindow->Write("Command or executable not found\n");
        }
        delete comboCMD;
    }
    return 0;
}