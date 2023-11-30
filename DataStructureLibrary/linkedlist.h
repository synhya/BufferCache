#ifndef EMBEDEDOS_LINKEDLIST_H
#define EMBEDEDOS_LINKEDLIST_H

typedef struct Node     //make Node for linked list using structure
{
    int value;            //value part of Node contains the element
    struct Node *next;    //the next part of Node contains the address of next element of list
}Node;

typedef struct LinkedList     //make Node for linked list using structure
{
    int current_size;
    int capacity;
    struct Node *head;
}LinkedList;


void list_setup(LinkedList* new_list,int capacity);
void list_insert_first(LinkedList* list, int ele);
void list_clear(LinkedList* list);

int list_pop_last(LinkedList* list);

int  list_contains_item(LinkedList* list, int ele);
int list_pop_item(LinkedList* list, int ele);

void insertlast(LinkedList* list, int ele);
void insertafter(LinkedList* list, int ele, int num);

void print(LinkedList* list);

void deletefirst(LinkedList* list);
void deleteitem(LinkedList* list, int ele);
void deletelast(LinkedList* list);
void reverselist(LinkedList* list);

#endif //EMBEDEDOS_LINKEDLIST_H
