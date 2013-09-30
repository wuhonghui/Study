/*
 * pthread_create
 * pthread_exit
 *
 * pthread_detach
 * pthread_self
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

void *thread_foo(void *args)
{
    printf("thread foo start\n");
    
    pthread_detach(pthread_self());

    printf("thread foo exit\n");
    pthread_exit(NULL);
}

pthread_mutex_t a=PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

int main()
{
    pthread_t tid;

    printf("main thread start\n");
    
    if (0 != pthread_create(&tid, NULL, thread_foo, NULL))
    {
        printf("create thread failed...\n");
        return -1;
    }
 
    //a chance to wait thread foo to excute
    usleep(2000000); 

    printf("main thread exit\n");

    return 0;
}
