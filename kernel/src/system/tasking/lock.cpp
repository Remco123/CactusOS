#include <system/tasking/lock.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::system;

extern "C" int TestAndSet(int newValue, int* ptr);

MutexLock::MutexLock()
{
    this->value = 0;
}
void MutexLock::Lock()
{
    while (TestAndSet(1, &this->value) == 1) {
        if(System::scheduler && System::scheduler->Enabled)
            System::scheduler->ForceSwitch();
        else
            asm ("pause");
    }
}
void MutexLock::Unlock()
{
    this->value = 0;
}