#include<stdio.h>
#include<stdlib.h>
#include "linkedlist.h"

//struct Node *head;    //contains the address of first element of linked list

void list_setup(LinkedList* list, int capacity)
{
    list->head = NULL; //initialize the beginning(head) of list to NULL
    list->current_size = 0;
    list->capacity = capacity;
}

void list_insert_first(LinkedList* list, int ele)   //inserts ele in linked list
{
    Node* head = list->head;
    list->current_size += 1;

    Node *New;
    New=(Node*)malloc(sizeof(Node));    //New named Node declared with current_size of Node declared before
    New->value=ele;       //inserts the new ele to the value part of Node New
    New->next=NULL;           //makes the next part of Node New NULL so that no garbage value remains
    New->next=head;         //the address of previously first Node, which was stored in head is now assigned to next part of Node New
    head=New;              //the address of new first ele which is present in Node New is assigned to head Node
}

void print(LinkedList* list)
{
    Node* head = list->head;

    if(head==NULL)    //condition to check whether list is empty
    {
        printf("list is empty\n");
        return;
    }
    struct Node *cur=head;
    int count;
    count=0;
    while(cur!=NULL)                  //the loop traverse until it gets any NULL Node
    {
        printf("%d->",cur->value);
        count++;                      //counts the number of nodes or elements present in list
        cur=cur->next;                //moves cur pointer to next Node to check and get value
    }
    printf("NULL\n");
    printf("number of nodes %d\n",count);
}

void deleteitem(LinkedList* list, int ele)
{
    Node* head = list->head;

    if(head==NULL)
        printf("list is empty and nothing to delete\n");
    struct Node* cur=head;
    struct Node* prev=NULL;
    while(cur->value!=ele)
    {
        prev=cur;
        cur=cur->next;
    }
    if(prev!=NULL)
        prev->next=cur->next;       //the address of next Node after the Node containing element to be deleted is assigned to the previous Node of the Node containing the element to be deleted
    free(cur);                      //memory of the structure cur is deallocated

    list->current_size -= 1;
}

/// return 1 if found
int list_contains_item(LinkedList* list, int ele)
{
    Node* head = list->head;

    struct Node* temp ;
    temp = head;
    while (temp != 0)
    {
        if (temp->value == ele)
            return 1 ;          //element is found
        temp = temp->next;
    }
    return 0 ;
}

void insertlast(LinkedList* list, int ele)    //insert at the last of linked list
{
    Node* head = list->head;

    struct Node *New, *temp;
    New = (struct Node*)malloc(sizeof(struct Node));
    if(New== NULL)
    {
        printf("Unable to allocate memory.");
        return;
    }
    else
    {
        New->value = ele;
        New->next = NULL;
        temp = head;
        while(temp->next != NULL)
            temp = temp->next;
        temp->next = New;

        list->current_size += 1;
    }
}

void deletelast(LinkedList* list)   //delete the last element
{
    Node* head = list->head;

    if(head==NULL)
    {
        printf("list is empty and nothing to delete\n");
        return;
    }
    struct Node* cur=head;
    struct Node* prev=NULL;
    while(cur->next!=NULL)
    {
        prev=cur;
        cur=cur->next;
    }
    if(prev->next!=NULL)
        prev->next=NULL;
    free(cur);

}

void deletefirst(LinkedList* list)    //delete the first element
{
    Node* head = list->head;

    struct Node* cur;
    if(head==NULL)
    {
        printf("list is empty and nothing to delete\n");
        return;
    }

    cur=head;
    head=head->next;
    free(cur);
    list->current_size -= 1;
}

void insertafter(LinkedList* list, int ele, int num)   //inserts element for any given element present in linked list
{
    Node* head = list->head;

    struct Node* New;
    New=(struct Node*)malloc(sizeof(struct Node));
    New->value=ele;
    New->next=NULL;
    struct Node* prev=head;
    while(prev->value!=num)
    {
        prev=prev->next;
    }
    New->next=prev->next;
    prev->next=New;
    list->current_size += 1;
}

void printReverse(LinkedList* list)    //print the linked list in reverse way using recursion
{
    Node* head = list->head;

    if (head == NULL)
        return;
    printReverse(head->next);
    printf("%d->", head->value);
}

void reverselist(LinkedList* list)    //reverse the linked list
{
    Node* head = list->head;

    struct Node* prev=NULL;
    struct Node* cur=head;
    struct Node* nxt;
    while(cur!=NULL)
    {
        nxt=cur->next;
        cur->next=prev;
        prev=cur;
        cur=nxt;
    }
    head=prev;      //points the head pointer to prev as it the new head or beginning in reverse list
}

void sum(LinkedList* list)    //sum of elements of the linked list
{
    Node* head = list->head;

    int s;
    struct Node *cur=head;
    s=0;
    while(cur!=NULL)
    {
        s+=cur->value;
        cur=cur->next;
    }
    printf("Sum of elements is %d\n",s);
}

int list_pop_last(LinkedList* list)   //delete the last element
{
    Node* head = list->head;
    int value;

    if(head==NULL)
    {
        printf("list is empty and nothing to delete\n");
        return -1;
    }
    struct Node* cur=head;
    struct Node* prev=NULL;
    while(cur->next!=NULL)
    {
        prev=cur;
        cur=cur->next;
    }
    if(prev->next!=NULL)
        prev->next=NULL;
    value = cur->value;
    free(cur);
    list->current_size -= 1;

    return value;
}

// find element and pop it out
int list_pop_item(LinkedList* list, int ele)
{
    Node* head = list->head;
    int value;

    if(head==NULL)
        printf("list is empty and nothing to delete\n");
    struct Node* cur=head;
    struct Node* prev=NULL;
    while(cur->value!=ele)
    {
        prev=cur;
        cur=cur->next;
    }
    if(prev!=NULL)
        prev->next=cur->next;       //the address of next Node after the Node containing element to be deleted is assigned to the previous Node of the Node containing the element to be deleted
    value = cur->value;
    free(cur);                      //memory of the structure cur is deallocated

    list->current_size -= 1;

    return value;
}

void list_clear(LinkedList* list) {
    while(list->head != NULL) {
        deletefirst(list);
    }
}