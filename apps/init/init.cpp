#include <log.h>
#include <api.h>

using namespace LIBCactusOS;

int main()
{
    API::Initialize();

    Log(LogLevel::Info, "Init process started!");    
    
    return 0;
}