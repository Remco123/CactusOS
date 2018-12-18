#include <system/components/systemcomponent.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;

SystemComponent::SystemComponent(char* name, char* description)
{
    this->Name = name;
    this->Description = description;
}

char* SystemComponent::GetComponentName()
{
    return this->Name;
}
char* SystemComponent::GetComponentDescription()
{
    return this->Description;
}