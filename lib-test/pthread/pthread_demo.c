#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <pthread.h>


void * thread_test(void *args)
{
    int index = 0;

    if (NULL != args)
    {
        index = *((int *)args);
        free(args);
    }

	sleep(index - 20);
    
    printf("thread id:%08x, index = %d\n", (int)pthread_self(), index);

    pthread_exit(0);
}
#define MAX_THREADS 5
pthread_t pid[MAX_THREADS];
pthread_attr_t thread_attr;

int createThread()
{
    int i = 0;
    int *index = NULL;

    if(pthread_attr_init(&thread_attr))
    {
        printf("init attr failed\n");
        return -1;
    }
    
    if(pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE))
    {
        printf("set attr detach state failed\n");
        return -1;
    }
    
    
    for (i = 0; i < MAX_THREADS; ++i)
    {
        index = (int *)malloc(sizeof(*index));
        *index = 20 + i;
        if(pthread_create(&pid[i], &thread_attr, thread_test, (void *)index))
        {
            printf("create thread failed!\n");
            return -1;
        }
    }

    for (i = 0; i < MAX_THREADS; ++i)
    {
        if (pthread_join(pid[i], NULL))
        {
            printf("join thread failed\n");
            return -1;
        }
    }

    return 0;
}


int main(void)
{
    createThread();
    return 0;
}
