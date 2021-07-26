#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <defs/int.h>

#ifndef ENV_WINDOWS
#include <mem.h>
#endif

template <class T> class Node
{
public:
    Node() { }

    T Value;
    Node *Next;
    Node *Prev;
};

template <class T> class LinkedList
{
public:
    //  Len is inital length
    LinkedList()
    {
        auto List = new Node<T>[Len];
        Head = &(List[0]);

        Node<T> *Iterator = Head->Next;
        for(UINT64 i = 1; i < Len; i++)
        {
            Iterator->Next = &(List[i]);
            Iterator = Iterator->Next;
        }

        Iterator->Next = nullptr;
    }

    Node<T> *Head;
    UINT64 Length;

    void Append(T Value)
    {

    }

    T operator[](UINT64 Index)
    {
        Node<T> *Iterator = Head;

        for(UINT64 i = 0; i < Index; i++) Iterator = Iterator->Next;

        return Iterator->Value;
    }
};

#endif
