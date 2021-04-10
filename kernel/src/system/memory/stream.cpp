#include <system/memory/stream.h>
#include <system/log.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;

Stream::Stream() {}
Stream::~Stream() {}

char Stream::Read()
{
    Log(Error, "Virtual stream function called");
    return 0;
}

void Stream::Write(char byte)
{
    Log(Error, "Virtual stream function called");
}

int Stream::Available()
{
    Log(Error, "Virtual stream function called");
    return 0;
}