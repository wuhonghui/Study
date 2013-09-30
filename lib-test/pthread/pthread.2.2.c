/*
 * pthread_mutex_init
 * pthread_mutex_lock
 * pthread_mutex_unlock
 * pthread_mutex_destory
 * pthread_mutex_trylock
 *
 * pthread_mutexattr_init
 * pthread_mutexattr_settype
 * pthread_mutexattr_destory
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>


static int counter = 0;
static pthread_mutex_t counter_mutex;

static void *thread_foo(void *args)
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

static void *thread_bar(void *args)
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

static int init_errorcheck_mutex(pthread_mutex_t *mutex)
{
    pthread_mutexattr_t attr;
    if(pthread_mutexattr_init(&attr))
    {
        printf("init mutex attr failed.\n");
        return -1;
    }
    if(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK))
    {
        printf("set attr type errorcheck failed.\n");
        return -1;
    }
    /* more attr... */

    if(pthread_mutex_init(mutex, &attr))
    {
        printf("init errorcheck mutex failed.\n");
        return -1;
    }

    if(pthread_mutexattr_destroy(&attr))
    {
        printf("destory mutex attr failed.\n");
        return -1;
    }

    return 0;
}

static int init_recursive_mutex(pthread_mutex_t *mutex)
{
    pthread_mutexattr_t attr;
    if(pthread_mutexattr_init(&attr))
    {
        printf("init mutex attr failed.\n");
        return -1;
    }

    if(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE))
    {
        printf("set attr type recursive failed.\n");
        return -1;
    }
    /* more attr... */

    if(pthread_mutex_init(mutex, &attr))
    {
        printf("init recursive mutex failed.\n");
        return -1;
    }

    if(pthread_mutexattr_destroy(&attr))
    {
        printf("destory mutex attr failed.\n");
        return -1;
    }

    return 0;
}


static void test_false_lock()
{
    int ret = 0;
    if (0 != (ret = pthread_mutex_trylock(&counter_mutex)))
    {
        printf("(1)trylock failed....%s\n", strerror(ret));
    }

    if (0 != (ret = pthread_mutex_trylock(&counter_mutex)))
    {
        printf("(2)trylock failed....%s\n", strerror(ret));
    }

    if (0 != (ret = pthread_mutex_lock(&counter_mutex)))
    {
        printf("(3)lock failed....%s\n", strerror(ret));
    }
}


int main()
{
    pthread_t tid_foo;
    pthread_t tid_bar;
    
    //default mutex attr: counter = PTHREAD_MUTEX_INITIALIZER;
//    if (0 != init_recursive_mutex(&counter_mutex))
    if (0 != init_errorcheck_mutex(&counter_mutex))
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

    test_false_lock();

    if (0 != pthread_mutex_destroy(&counter_mutex))
    {
        printf("destory mutex failed...\n");
        return -1;
    }

    return 0;
}
