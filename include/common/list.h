#ifndef __CACTUSOS__COMMON__LIST_H
#define __CACTUSOS__COMMON__LIST_H


#include <common/types.h>
#include <common/convert.h>
#include <common/memoryoperations.h>
#include <core/memorymanagement.h>

namespace CactusOS
{
    namespace common
    {
        struct ListNode
        {
            void* data;
            ListNode* next;
        };

        class List
        {
        private:
            ListNode* head;
            ListNode* tail;
        public:
            int Length;
            List();
            ~List();

            void AddStart(void* value);
            void AddEnd(void* value);
            void AddAt(int pos, void* value);

            void DeleteFirst();
            void DeleteAt(int pos);
            void DeleteEnd();
        };
    }
}


#endif