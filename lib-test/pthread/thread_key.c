#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <pthread.h>

pthread_once_t key_once_init = PTHREAD_ONCE_INIT;
pthread_key_t key;

void free_str_buf(void *buf)
{
    if(NULL != buf)
    {
        free(buf);
    }
}

void init_key(void)
{
    pthread_key_create(&key, free_str_buf);
}

int output_specific(void)
{
    printf("thread name: %s\n", pthread_getspecific(key));
    return 0;
}

#define THREAD_NAME_MAX_LEN 30
void * thread_test(void *args)
{
    int seq = (int)args;
    char *thread_name = NULL;
    
    pthread_once(&key_once_init, init_key);

    thread_name = (char *)malloc(THREAD_NAME_MAX_LEN);
    memset(thread_name, 0, THREAD_NAME_MAX_LEN);

    pthread_setspecific(key, (void *)thread_name);

    snprintf(thread_name, THREAD_NAME_MAX_LEN, "%s %d %08x", __FUNCTION__, seq, pthread_self());

    output_specific();
    
    int *ret= (int *)malloc(sizeof(int));
    *ret = seq * -1;
    pthread_exit((void *)ret);
}


#define MAX_THREAD 10
pthread_t tid[MAX_THREAD];

int main(void)
{
    int i = 0;
    int *thread_return_ptr = NULL;

    for(i = 0; i < MAX_THREAD; ++i)
    {
        if (pthread_create(&tid[i], NULL, thread_test, (void *)i))
        {
            printf("create thread %d failed\n", i);
        }
    }

    for(i = 0; i < MAX_THREAD; ++i)
    {
        if (pthread_join(tid[i], (void **)&thread_return_ptr))
        {
            printf("join thread %d failed\n", i);
        }
        else
        {
            printf("thread %d %08x return %d\n", i, tid[i], *thread_return_ptr); 
            free(thread_return_ptr);
        }
    }

    if (pthread_key_delete(key))
    {
        printf("delete key failed\n");
    }

    return 0;
}
