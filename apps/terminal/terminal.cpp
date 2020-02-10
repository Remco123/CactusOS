#include <new.h>
#include <gui/gui.h>
#include <gui/widgets/window.h>
#include <gui/widgets/label.h>
#include <gui/widgets/inputbox.h>
#include <gui/directgui.h>
#include <string.h>
#include <log.h>
#include <proc.h>
#include <convert.h>
#include <vfs.h>
#include "terminalcontrol.h"

char* workingDir = "B:\\";
TerminalControl* termWindow = 0;

int ExecCommand(char* cmd);
int main()
{
    Window* mainWindow = new Window(600, 400, WIDTH/2 - 300, HEIGHT/2 - 200);
    mainWindow->titleString = "CactusOS Terminal";

    termWindow = new TerminalControl(mainWindow->width, mainWindow->height - 30);
    mainWindow->AddChild(termWindow);
    
    GUI::MakeAsync();
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

// Excecute command and return pid of created process if necessary
int ExecCommand(char* cmd)
{
    if(memcmp(cmd, "ls", 3) == 0)
    {
        List<char*> items = DirectoryListing(workingDir);
        for(char* item : items)
        {
            termWindow->Write(item); termWindow->Write('\n');
            delete item;
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
        List<ProcessInfo*> items = Process::GetProcessList();
        for(ProcessInfo* item : items)
        {
            termWindow->Write(item->fileName);
            for(int i = 0; i < (32-strlen(item->fileName)); i++)
                termWindow->Write(' ');

            termWindow->Write(" PID=");
            termWindow->Write(Convert::IntToString(item->id));
            termWindow->Write(" Treads ");
            termWindow->Write(Convert::IntToString(item->threads));
            termWindow->Write(item->blocked ? (char*)" Blocked" : (char*)" !Blocked");
            termWindow->Write(" Memory ");
            termWindow->Write(Convert::IntToString(item->heapMemory / 1024));
            termWindow->Write(" Kb");
            termWindow->Write('\n');
            delete item;
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
    else if(memcmp(cmd, "ldir", 4) == 0) {
        List<DiskInfo> items = DiskListing();
        for(DiskInfo item : items) {
            termWindow->Write(item.identifier); termWindow->Write('\n');
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