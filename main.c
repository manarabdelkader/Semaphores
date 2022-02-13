#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#define N 5
#define buffersize 5


int buffer[buffersize];
sem_t count_sem,buffer_sem, usedSlots, emptySlots;

int count=0;
void* counterFunc(void* args)
{
    int counterID = *((int*) args);

    while(1)
    {
        printf("Counter thread %d : received a message\n",counterID);
        int val;
        sem_getvalue(&count_sem,&val);   //check mutual exclusion
        if (val<=0)
        {
            printf("Counter thread %d : waiting to write\n",counterID);
        }

        sem_wait(&count_sem);  //enter critical section
        count++;
        printf("Counter thread %d : now adding to counter,counter value= %d\n",counterID,count);
        sem_post(&count_sem);  //exit critical section

        srand(time(0));
        int t = rand()%100;
        sleep(t);
    }
}


void* monitorFunc(void* args)
{
    int in=0;
    while(1)
    {
        int val;  //check mutual exclusion
        sem_getvalue(&count_sem,&val);
        if (val<=0)
        {
            printf("Monitor thread : waiting to read counter\n");
        }

        sem_wait(&count_sem);  //enter critical section
        printf("Monitor thread : reading a count value of %d\n",count);
        int tempcount=count;
        count=0;
        sem_post(&count_sem);  //exit critical section


        int emptyslots;
        sem_getvalue(&emptySlots, &emptyslots);
        if(emptyslots<= 0)
        {
            printf("Monitor thread: buffer is full\n");  //no empty slots so buffer is full
        }

        sem_wait(&emptySlots);   //enter if available empty slots
        int bufferval;
        sem_getvalue(&buffer_sem, &bufferval);  //check mutual exclusion
        if(bufferval <= 0)
        {
            printf("Monitor thread: waiting for buffer\n");
        }

        sem_wait(&buffer_sem);  //enter critical section
        buffer[in]=tempcount;
        printf("Monitor thread: writing to buffer at position %d\n",in+1);
        in++;
        if(in==buffersize)
        { in=0;
        }
        sem_post(&buffer_sem);  //exit critical section
        sem_post(&usedSlots);   //increment used slots

        srand(time(0));
        int t= rand()%10;
        sleep(t);
    }
}



void* collectorFunc(void* args)
{
    int out=0;
    while(1)
    {
        int usedslots;
        sem_getvalue(&usedSlots, &usedslots);
        if(usedslots <=0)
        {
            printf("Collector thread: buffer is empty\n"); //no used slots so buffer is empty
        }

        sem_wait(&usedSlots);  //enter if buffer is not empty
        int bufferval;
        sem_getvalue(&buffer_sem, &bufferval);  //check mutual exclusion
        if(bufferval <= 0)
        {
            printf("Collector thread: waiting for buffer\n");
        }

        sem_wait(&buffer_sem);  //enter critical section
        printf("Collector thread: reading from buffer at position %d\n",out+1);
        out++;
        if(out==buffersize)
        {out=0;
        }
        sem_post(&buffer_sem);  //exit critical section
        sem_post(&emptySlots);  //increment empty slots


        srand(time(0));
        int t= rand();
        sleep(t)%10;
    }
}


int main()
{
    sem_init(&count_sem, 1, 1);
    sem_init(&buffer_sem, 1, 1);
    sem_init(&usedSlots, 1, 0);
    sem_init(&emptySlots, 1, buffersize);

    pthread_t counter_threads[N], monitor_thread, collector_thread;

    for(int i=0; i<N; i++)
    {
        int* cID = (int*) malloc(sizeof(int));
        *cID = i + 1;
        pthread_create(&counter_threads[i], NULL, counterFunc, (void*) cID);
    }

    pthread_create(&monitor_thread, NULL, monitorFunc, NULL);
    pthread_create(&collector_thread, NULL, collectorFunc, NULL);

    pthread_join(collector_thread, NULL);
    return 0;
}
