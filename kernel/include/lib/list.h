#ifndef LIB_LIST_H
#define LIB_LIST_H

#include <stddef.h>

struct list_head {
    struct list_head* prev;
    struct list_head* next;
};

#define LIST_NEXT(type, var)          ((type*)((struct list_head*)var)->next)
#define LIST_PREV(var)                ((typeof(var))((struct list_head*)var)->prev)
#define LIST_FOREACH(head)            for(struct list_head* p = (struct list_head*)head; p != NULL; p = p->next)

#endif // LIB_LIST_H

