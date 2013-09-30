/*
 * pthread_cond_init
 * pthread_cond_destory
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

pthread_cond_t time_to_ring = PTHREAD_COND_INITIALIZER;
pthread_mutex_t time_to_ring_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t time_to_eat = PTHREAD_COND_INITIALIZER;
pthread_mutex_t time_to_eat_mutex = PTHREAD_MUTEX_INITIALIZER;

void *thread_master(void *args)
{
    pthread_detach(pthread_self());

    while(1)
    {
        sleep(8);

        printf("\nMaster: press the ring button.\n");
        pthread_cond_signal(&time_to_ring);
    }

    pthread_exit(NULL);
}


void *thread_ring(void *args)
{
    pthread_detach(pthread_self());

    while(1)
    {
        pthread_mutex_lock(&time_to_ring_mutex);
        pthread_cond_wait(&time_to_ring, &time_to_ring_mutex);
        pthread_mutex_unlock(&time_to_ring_mutex);
        
        printf("Ring: begin ringing...\n");

        pthread_cond_broadcast(&time_to_eat); //all dog to eat
        //pthread_cond_signal(&time_to_eat); //only one dog to eat
    }

    pthread_exit(NULL);
}


void *thread_dog(void *args)
{
    pthread_detach(pthread_self());

    int i = (int)args;

    while(1)
    {
        printf("dog %d is hungry, wait to eat...\n", i);
        pthread_mutex_lock(&time_to_eat_mutex);
        pthread_cond_wait(&time_to_eat, &time_to_eat_mutex);
        pthread_mutex_unlock(&time_to_eat_mutex);
        
        int num = random()%6 + 5; //5-10
        printf("dog %d hear the ring, eat %d food.\n", i, num);

        sleep(num);
    }

    pthread_exit(NULL);
}


int main()
{
    pthread_t tid_master;
    pthread_t tid_ring;
    pthread_t tid_dog[5];
    
    if (0 != pthread_create(&tid_master, NULL, thread_master, NULL))
    {
        printf("create thread master failed...\n");
        return -1;
    }

    if (0 != pthread_create(&tid_ring, NULL, thread_ring, NULL))
    {
        printf("create thread ring failed...\n");
        return -1;
    }
    
    int i = 0;
    for (i = 0; i < 5; i++)
    {
        if (0 != pthread_create(&tid_dog[i], NULL, thread_dog, (void *)i))
        {
            printf("create thread dog %d failed...\n", i);
            return -1;
        }
    }

    sleep(1000);

    return 0;
}
