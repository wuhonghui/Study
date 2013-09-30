#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>


int init_default_mutex(pthread_mutex_t *mutex)
{
    pthread_mutexattr_t attr;
    if(pthread_mutexattr_init(&attr))
    {
        printf("init mutex attr failed.\n");
        return -1;
    }
    if(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL))
    {
        printf("set attr type normal(default) failed.\n");
        return -1;
    }
    /* more attr to add here? */

    if(pthread_mutex_init(mutex, &attr))
    {
        printf("init default mutex failed.\n");
        return -1;
    }

    return 0;
}

int init_recursive_mutex(pthread_mutex_t *mutex)
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
    /* more attr to add here? */

    if(pthread_mutex_init(mutex, &attr))
    {
        printf("init recursive mutex failed.\n");
        return -1;
    }

    return 0;
}

int init_errorcheck_mutex(pthread_mutex_t *mutex)
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
    /* more attr to add here? */

    if(pthread_mutex_init(mutex, &attr))
    {
        printf("init errorcheck mutex failed.\n");
        return -1;
    }

    return 0;
}

int init_adaptive_mutex(pthread_mutex_t *mutex)
{
    pthread_mutexattr_t attr;
    if(pthread_mutexattr_init(&attr))
    {
        printf("init mutex attr failed.\n");
        return -1;
    }
    if(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ADAPTIVE_NP))
    {
        printf("set attr type adaptive failed.\n");
        return -1;
    }
    /* more attr to add here? */

    if(pthread_mutex_init(mutex, &attr))
    {
        printf("init errorcheck mutex failed.\n");
        return -1;
    }

    return 0;
}

pthread_mutex_t timed_mutex;
pthread_mutex_t recursive_mutex;
pthread_mutex_t errorcheck_mutex;
pthread_mutex_t adaptive_mutex;

int init_mutex(void)
{
    printf("default type %d\n", PTHREAD_MUTEX_DEFAULT);
    printf("normal: %d\n", PTHREAD_MUTEX_NORMAL);
    printf("recursive: %d\n", PTHREAD_MUTEX_RECURSIVE);
    printf("errorcheck: %d\n", PTHREAD_MUTEX_ERRORCHECK);
    
    printf("timed_np: %d\n", PTHREAD_MUTEX_TIMED_NP);
    printf("recursive_np: %d\n", PTHREAD_MUTEX_RECURSIVE_NP);
    printf("errorcheck_np: %d\n", PTHREAD_MUTEX_ERRORCHECK_NP);
    printf("adaptive_np: %d\n", PTHREAD_MUTEX_ADAPTIVE_NP);

    if(init_default_mutex(&timed_mutex))
    {
        return -1;
    }
    if(init_errorcheck_mutex(&errorcheck_mutex))
    {
        return -1;
    }
    if(init_recursive_mutex(&recursive_mutex))
    {
        return -1;
    }
    if(init_adaptive_mutex(&adaptive_mutex))
    {
        return -1;
    }

    return 0;
}

int create_threads(void)
{
    return 0;
}

int main(int argc, char *argv[])
{

    if(init_mutex())
    {
        printf("init mutex failed.\n");
        return -1;
    }

    
    if(create_threads())
    {
        printf("create threads failed.\n");
        return -1;
    }

    return 0;
}
