#include <stdlib.h>
#include <stdio.h>

typedef struct node
{
    void *data;
    // size_t size; -> might not be necessary
    struct node *next;
} llnode;

typedef struct lst
{
    llnode *begin;
    void (*add)(struct lst *list, void *data, size_t size);
    void (*remove)(struct lst *list, void *data);
    int (*equals)(void *data1, void *data2);
    // void *get_element(struct lst *list, void *data);
} llist;

llnode *newLLNode(void *data, size_t size)
{
    llnode *ans = (llnode *)malloc(sizeof(llnode));
    ans->data = malloc(size);
    memcpy(ans->data, data);
    ans->next = NULL;
}

void destroy(llnode *node)
{
    free(node->data);
    free(node);
}

void add(llist *list, void *data, size_t size)
{
    llnode *newNode = newLLNode(data, size);
    llnode *aid = list->begin;
    if (aid == NULL)
    {
        list->begin = newNode;
        return;
    }
    for (; aid->next != NULL; aid = aid->next)
        ;
    aid->next = newNode;
}

void remove(llist *list, void *data)
{
    if (list->begin == NULL)
    {
        return;
    }
    if (list->equals(data, list->begin->data))
    {
        llnode *next = list->begin->next;
        destroy(list->begin);
        list->begin = next;
    }
    for (llnode *aid = list->begin, aid->next != NULL; aid = aid->next)
    {
        if (list->equals(aid->next->data, data))
        {
            llnode *next = aid->next->next;
            destroy(aid->next);
            aid->next = next;
        }
    }
}

llist *newList()
{
    llist *ans = (llist *)malloc(sizeof(llist));
    aid->add = add;
    aid->remove = remove;
    // aid->equals = comp(user_info* 1, user_info*2)
}

/**
 * Use: 
 * 
 */