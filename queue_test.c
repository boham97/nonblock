#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>  
#include "queue.h"
#include <unistd.h>

#define THREADS 5
#define OPS     20000

Queue q;


// cc -g -o queue queue.c queue_test.c -lpthread



void* producer(void* arg) 
{
    int id = (intptr_t)arg;
    for (int i = 0; i < OPS; i++) 
    {
        enqueue(&q, (void*)(intptr_t)(id * OPS + i + 1 ));
        // printf("th %d append: %d\n", id,  id * OPS + i + 1);
    }
    return NULL;
}

void* consumer(void* arg) 
{
    int id = (intptr_t)arg;
    int cnt = 0;
    int pop_count = 0;
    Node* n;
    while (pop_count < OPS) 
    {
        n = dequeue(&q);
        if (n)
        {
            cnt = (int)(intptr_t)(n->value);
            free(n);
            pop_count++;
            // printf("th:%d pop: %d\n", id, cnt);
            usleep(1);
        }
        else
            usleep(1);
    }
    return NULL;
}

int main() {
    initQueue(&q);

    pthread_t prod[THREADS], cons[THREADS];

    for (int i = 0; i < THREADS; i++) 
    {
        pthread_create(&prod[i], NULL, producer, (void*)(intptr_t)i);
        pthread_create(&cons[i], NULL, consumer, (void*)(intptr_t)i);
    }

    for (int i = 0; i < THREADS; i++) 
    {
        pthread_join(prod[i], NULL);
    }
    for (int i = 0; i < THREADS; i++) 
    {
        pthread_join(cons[i], NULL);
    }

    printf("All threads finished.\n");
    if(dequeue(&q))printf("something wrong\n");
    // if(dequeue(&q))printf("something wrong\n");
    return 0;
}
