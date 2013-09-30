/*
 * pthread_create
 * pthread_exit
 *
 * pthread_calcel
 * pthread_join
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>


void *thread_foo(void *args)
{
    printf("thread foo start\n");
    sleep(5);  //wait for main to datach and cancel
    printf("thread foo exit\n");
    pthread_exit(NULL);
}

int main()
{
    pthread_t tid;

    printf("main thread...start\n");
    if (0 != pthread_create(&tid, NULL, thread_foo, NULL))
    {
        printf("create thread failed...\n");
        return -1;
    }
    
    sleep(1);
    printf("main thread cancel thread foo\n");
    if (0 != pthread_cancel(tid))
    {
        printf("cancel thread failed...\n");
        return -1;
    }

    if (0 != pthread_join(tid, NULL))
    {
        printf("join thread failed...\n");
        return -1;
    }

    return 0;
}
