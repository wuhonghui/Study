#ifndef stack_H
#define stack_H

#define T stack_t
typedef struct T *T;

extern T     stack_new  (void);
extern int   stack_empty(T stk);
extern void  stack_push (T stk, void *x);
extern void *stack_pop  (T stk);
extern void  stack_free (T *stk);

#undef T

#endif
