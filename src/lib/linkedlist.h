
#ifndef _LINKEDLIST_H
#define _LINKEDLIST_H

#include <defs/int.h>
#include <mem.h>

template <class T> class Node
{
public:
    Node() { }

    T Value;
    Node *Next;
};

template <class T> class LinkedList
{
public:
    //  Len is inital length
    LinkedList(UINT64 Len)
    {
        auto List = new Node<T>[Len];
        First = &(List[0]);

        Node<T> *Iterator = First->Next;
        for(UINT64 i = 1; i < Len; i++)
        {
            Iterator->Next = &(List[i]);
            Iterator = Iterator->Next;
        }

        Iterator->Next = nullptr;
    }

    Node<T> *First;
    UINT64 Length;

    void Append(T Value)
    {

    }

    T operator[](UINT64 Index)
    {
        Node<T> *Iterator = First;

        for(UINT64 i = 0; i < Index; i++) Iterator = Iterator->Next;

        return Iterator->Value;
    }
};

#endif
