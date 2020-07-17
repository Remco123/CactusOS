#include "contextmanager.h"

using namespace LIBCactusOS;

ContextManager::ContextManager()
: contextList()
{ }

ContextManager::~ContextManager()
{ }

ContextInfo* ContextManager::FindTargetContext(int x, int y)
{
    // Loop through all known contexts starting at the one on the front
    for(ContextInfo* c : contextList)
    {
        if(x >= c->x && x <= c->x + c->width)       // Check if the coordinate x,y fits in the border
            if(y >= c->y && y <= c->y + c->height)  // of the context. If so return as result.
                return c;
    }

    return 0;
}

void ContextManager::MoveToFront(ContextInfo* info)
{
    contextList.Remove(info);
    contextList.push_front(info);
}