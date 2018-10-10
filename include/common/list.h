#ifndef __CACTUSOS__COMMON__LIST_H
#define __CACTUSOS__COMMON__LIST_H

#include <common/string.h>

namespace CactusOS
{
    namespace common
    {
        template <typename T>
        struct ListNode
        {
            ListNode(const T &e) : data(e), next(0), prev(0)
            {}

            T data;
            ListNode<T>* next;
            ListNode<T>* prev;
        };


        template <typename T>
        class List
        {
        public:
            List() : head_(0), tail_(0), size_(0)
            {   }
            ~List()
            { this->Clear(); /*Remove all the items from the list*/ }

            int size() { return size_; }
            void push_back(const T &e);
            void push_front(const T &e);
            void Clear();

            T GetAt(int index);
            T operator[](int index);

        private:
            ListNode<T>* head_;
            ListNode<T>* tail_;

            int size_;

            ListNode<T>* insertInternal(const T &e, ListNode<T>* pos);
        };
    }
}

using namespace CactusOS::common;

/////////////
// Implementations
////////////
template <typename T>
ListNode<T>* List<T>::insertInternal(const T &e, ListNode<T>* pos)
{
    ListNode<T> *n = new ListNode<T>(e);
    size_++;
    // no operation below this should throw
    // as state of the list has changed and memory allocated
    n->next = pos;
    if(pos)
    {
        n->prev = pos->prev;
        pos->prev = n;
    }
    else
    {
        // pos is null that is at the very end of the list
        n->prev = tail_;
        tail_ = n;
    }
    if(n->prev)
    {
        n->prev->next = n;
    }
    else
    {
        // at the very begining of the list
        head_ = n;
    }
    return n;
}
template <typename T>
void List<T>::push_back(const T &e)
{
    // inserts before the position, 
    // 0 is the end() iterator
    // hence insert at the end
    insertInternal(e, 0);
}
template <typename T>
void List<T>::push_front(const T &e)
{
    // insert before the head
    insertInternal(e, head_);
}

template <typename T>
void List<T>::Clear()
{
    ListNode<T>* current( head_ );

    while(current)
    {
        ListNode<T>* next( current->next );
        delete current;
        current = next;
    }
    size_ = 0; //Reset the size to 0
}

template <typename T>
T List<T>::GetAt(int index)
{
    if(index == 0)
        return this->head_->data;
    else
    {
        ListNode<T>* cur = head_;
        for(int i = 0; i < index; ++i)
            cur = cur->next;
        return cur->data;
    }
}

template <typename T>
T List<T>::operator[](int index)
{
    return GetAt(index);
}

#endif