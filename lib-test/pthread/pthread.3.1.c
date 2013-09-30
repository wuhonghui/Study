/*
 * pthread_rwlock_init
 * pthread_rwlock_destory
 * pthread_rwlock_rdlock
 * pthread_rwlock_wrlock
 * pthread_rwlock_unlock
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define SCREEN_BUFFER_LEN   256
struct Screen{
    char buffer[SCREEN_BUFFER_LEN];
    int bufferlen;
    pthread_rwlock_t rwlock;
};

struct Screen screen;

static void *thread_teacher(void *args)
{
    pthread_detach(pthread_self());

    while (1)
    {
        pthread_rwlock_wrlock(&screen.rwlock);
        {
            snprintf(screen.buffer, SCREEN_BUFFER_LEN, "%d %d", 
                    (int)random()%100000000, (int)random()%100000000);
            usleep(1000000);
            screen.bufferlen = strlen(screen.buffer);
            
            printf("\n<M>teacher write %s, len: %d\n", screen.buffer, screen.bufferlen);
        }
        pthread_rwlock_unlock(&screen.rwlock);
        
        usleep((random()%1000000) + 2000000); // 2.0s ~ 3.0s
    }

    pthread_exit(NULL);
}

static void *thread_student(void *args)
{
    int i = (int)args;
    pthread_detach(pthread_self());

    usleep(3000000);

    while(1)
    {
        pthread_rwlock_rdlock(&screen.rwlock);
        {
            printf("student %d read %s, len %d\n", i, screen.buffer, screen.bufferlen);
            usleep(200);
        }
        pthread_rwlock_unlock(&screen.rwlock);

        usleep((random()%1500000) + 1500000); // 1.5s ~ 3.0s
    }

    pthread_exit(NULL);
}

int init_screen()
{
    memset(&screen, 0, sizeof(struct Screen));

    if (pthread_rwlock_init(&screen.rwlock, NULL))
    {
        printf("init rwlock failed...\n");
        return -1;
    }

    return 0;
}

int main()
{
    pthread_t tid_teacher;
    pthread_t tid_student[10];
    
    //default mutex attr: counter = PTHREAD_MUTEX_INITIALIZER;
//    if (0 != init_recursive_mutex(&counter_mutex))
    if (0 != init_screen())
    {
        printf("init screen failed...\n");
        return -1;
    }

    if (0 != pthread_create(&tid_teacher, NULL, thread_teacher, NULL))
    {
        printf("create thread teacher failed...\n");
        return -1;
    }
    
    int i = 0;
    for (i = 0; i < 10; i++)
    {
        if (0 != pthread_create(&tid_student[i], NULL, thread_student, (void *)i))
        {
            printf("create thread student %d failed...\n", i);
            return -1;
        }
    }

    if (0 != pthread_rwlock_destroy(&screen.rwlock))
    {
        printf("destory rwlock failed...\n");
        return -1;
    }

    sleep(100);

    return 0;
}
