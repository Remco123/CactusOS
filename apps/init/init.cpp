#include <log.h>
#include <api.h>

using namespace LIBCactusOS;

int main()
{
    API::Initialize();

    Log(LogLevel::Info, "Init process started!");

    int c = 0;
    while(1){
        Log(Info, "Process still active");
        for(int i = 0; i < 520000; i++)
            asm volatile("pause");

        if(c++ >= 30)
            return -1;
    }
    
    
    return 0;
}