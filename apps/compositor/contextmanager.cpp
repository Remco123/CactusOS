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
        if(x >= c->x && x <= c->x + (int)c->width)       // Check if the coordinate x,y fits in the border
            if(y >= c->y && y <= c->y + (int)c->height)  // of the context. If so return as result.
                return c;
    }

    return 0;
}
List<ContextInfo*> ContextManager::FindTargetContexts(Rectangle area)
{
    List<ContextInfo*> result;
    for(ContextInfo* c : contextList)
    {
        Rectangle item = Rectangle(c->width, c->height, c->x, c->y);
        if(area.Intersect(item, 0))
            result.push_back(c);
    }
    return result;
}

void ContextManager::MoveToFront(ContextInfo* info)
{
    contextList.Remove(info);
    contextList.push_front(info);
}