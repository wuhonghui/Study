#include <stdlib.h>
#include <assert.h>
#include "stack.h"

#define T stack_t
struct T {
    int count;
    struct elem {
        void *x;
        struct elem *next;
    }*head;
};

T stack_new(void)
{
    T stk;
    stk = malloc(sizeof *(stk));
    assert(stk != NULL);
    stk->count = 0;
    stk->head = NULL;
    return stk;
}

int stack_empty(T stk)
{
    assert(stk);   
    return (stk->count == 0);
}

void stack_push(T stk, void *x)
{
    struct elem *t;
    assert(stk);
    t = malloc(sizeof *(t));
    assert(t);
    t->x = x;
    t->next = stk->head;
    stk->head = t;
    stk->count++;
}

void *stack_pop(T stk)
{
    void *x;
    struct elem *t;
    assert(stk);
    assert(stk->count > 0);
    t = stk->head;
    stk->head = t->next;
    stk->count--;
    x = t->x;
    free(t);
    return x;
}

void stack_free(T *stk)
{
    struct elem *t;
    assert(stk && *stk);
    while ((*stk)->head != NULL) {
        t = (*stk)->head;
        (*stk)->head = t->next;
        free(t);
    }
    free(*stk);
    *stk = NULL;
}
