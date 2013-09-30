/*
 * pthread_
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

void *thread_shuttle(void *args);
void *thread_rocket(void *args);

/* NASA send to two aircrafts to space, aircraft A is a space shuttle, aircraft B is a rocket. */
void *thread_nasa(void *args)
{
    pthread_detach(pthread_self());

    pthread_t tid_shuttle;
    pthread_t tid_rocket;

    /* send rocket */
    if (0 != pthread_create(&tid_rocket, NULL, thread_rocket, NULL))
    {
        printf("nasa send rocket failed...\n");
    }
    else
    {
        printf("nasa send rocket to space~\n");
    }

    /* send space shuttle */
    if (0 != pthread_create(&tid_shuttle, NULL, thread_shuttle, NULL))
    {
        printf("nasa send shuttle failed...\n");
    }
    else
    {
        printf("nasa send shuttle to space~\n");
    }


    sleep(10);

    printf("nasa call back shuttle\n");
    pthread_cancel(tid_shuttle);
    if (pthread_join(tid_shuttle, NULL))
    {
        printf("wait shuttle back failed.\n");
    }
    else
    {
        printf("shuttle come back successfully.\n");
    }

    pthread_exit(NULL);
}


void *thread_shuttle(void *args)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    int old_cancel_type = -1;
    static pthread_mutex_t controller = PTHREAD_MUTEX_INITIALIZER;

    while(1)
    {
        printf("shuttle is flying~\n");
      //pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old_cancel_type); //cancel between cleanup_push and mutex_lock will failed.
        pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old_cancel_type); //shuttle will continue to run until it find a cancel point.
        {
            pthread_cleanup_push((void (*)(void *))pthread_mutex_unlock, (void *)&controller);
                sleep(1);   // for cancel may happen at here.
            pthread_mutex_lock(&controller);
                sleep(3);
            pthread_mutex_unlock(&controller);
                sleep(1);   // for cancel may happen at here.
            pthread_cleanup_pop(0);
        }
        pthread_setcanceltype(old_cancel_type, NULL);
    }

    pthread_exit(NULL);
}

/* a rocket, can not canceled... */
void *thread_rocket(void *args)
{
    pthread_detach(pthread_self());
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    while(1)
    {
        printf("rocket is flying~\n");
        sleep(3);
    }

    pthread_exit(NULL);
}

int main()
{
    pthread_t tid_nasa;
    
    if (0 != pthread_create(&tid_nasa, NULL, thread_nasa, NULL))
    {
        printf("create thread nasa failed...\n");
        return -1;
    }

   
    if (pthread_join(tid_nasa, NULL))
    {
        printf("join thread_nasa  failed.\n");
    }

    return 0;
}
