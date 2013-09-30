/*
 * pthread_mutex_init
 * pthread_mutex_lock
 * pthread_mutex_unlock
 * pthread_mutex_destory
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>


int counter = 0;
pthread_mutex_t counter_mutex;

void *thread_foo(void *args)
{
    int i = 0;
    for(i = 0; i < 5; i++)
    {
        pthread_mutex_lock(&counter_mutex);
        {
            counter = counter + 1;
            printf("foo: counter is %d\n", counter);
            fflush(NULL);
        }
        pthread_mutex_unlock(&counter_mutex);
    }

    pthread_exit(NULL);
}

void *thread_bar(void *args)
{
    int i = 0;
    for(i = 0; i < 5; i++)
    {
        pthread_mutex_lock(&counter_mutex);
        {
            counter = counter + 1;
            printf("bar: counter is %d\n", counter);
            fflush(NULL);
        }
        pthread_mutex_unlock(&counter_mutex);
    }

    pthread_exit(NULL);
}

int main()
{
    pthread_t tid_foo;
    pthread_t tid_bar;
    
    //default mutex attr: counter = PTHREAD_MUTEX_INITIALIZER;
    if (0 != pthread_mutex_init(&counter_mutex, NULL))
    {
        printf("init mutex failed...\n");
        return -1;
    }

    if (0 != pthread_create(&tid_foo, NULL, thread_foo, NULL))
    {
        printf("create thread foo failed...\n");
        return -1;
    }
    if (0 != pthread_create(&tid_bar, NULL, thread_bar, NULL))
    {
        printf("create thread bar failed...\n");
        return -1;
    }

    if (0 != pthread_join(tid_foo, NULL))
    {
        printf("join thread foo failed...\n");
        return -1;
    }
    if (0 != pthread_join(tid_bar, NULL))
    {
        printf("join thread bar failed...\n");
        return -1;
    }

    if (0 != pthread_mutex_destroy(&counter_mutex))
    {
        printf("destory mutex failed...\n");
        return -1;
    }

    return 0;
}
