#include <common/list.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;

void printf(char*);
void printfHex32(uint32_t);

List::List()
{
    this->head = 0;
    this->tail = 0;
    this->Length = 0;
}
List::~List()
{

}

void List::AddStart(void* value)
{
    ListNode* temp = new ListNode();
    temp->data = value;
    temp->next = head;
    head = temp;

    Length++;
}

void List::AddEnd(void* value)
{
    ListNode* temp = new ListNode();
    temp->data = value;
    temp->next = 0;
    if(head == 0)
    {
        head = temp;
        tail = temp;
        temp = 0;
    }
    else
    {	
        tail->next=temp;
        tail=temp;
    }

    Length++;
}

void List::AddAt(int pos, void* value)
{
    if(pos == 0)
        AddStart(value);
    else if(pos == Length - 1)
        AddEnd(value);
    else
    {
        ListNode* pre = new ListNode();
        ListNode* cur = new ListNode();
        ListNode* temp = new ListNode();
        cur = head;
        for(int i=0;i<pos;i++)
        {
            pre = cur;
            cur = cur->next;
        }
        temp->data = value;
        pre->next = temp;	
        temp->next = cur;

        Length++;
    }
}
void List::DeleteFirst()
{
    ListNode* temp = new ListNode();
    temp = head;
    head = head->next;
    delete temp;

    Length--;
}
void List::DeleteAt(int pos)
{
    if(pos == 0)
        DeleteFirst();
    else if(pos == Length - 1)
        DeleteEnd();
    else
    {
        ListNode* current = new ListNode();
        ListNode* previous = new ListNode();
        current = head;
        for(int i = 0; i < pos; i++)
        {
            previous = current;
            current = current->next;
        }
        previous->next = current->next;
            
        Length--;
    }
}
void List::DeleteEnd()
{
    ListNode* current = new ListNode();
    ListNode* previous = new ListNode();
    current = head;
    while(current->next != 0)
    {
        previous = current;
        current = current->next;	
    }
    tail = previous;
    previous->next = 0;
    delete current;

    Length--;
}