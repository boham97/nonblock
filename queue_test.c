#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>  
#include "queue.h"
#include <unistd.h>
#include <sched.h>
#include <time.h>

// 측정 코드 추가
struct timespec start, end;


#define THREADS 12                          //6 보다 12가 ops 높음
#define OPS     400000

Queue q;


// cc -g -o queue queue.c queue_test.c -lpthread



void* producer(void* arg) 
{
    int id = (intptr_t)arg;
    for (int i = 0; i < OPS; i++) 
    {
        enqueue(&q, (void*)(intptr_t)(id * OPS + i + 1 ));
    }
    // printf("th %d append: %d\n", id,  id * (OPS + 1) + 1);
    return NULL;
}

void* consumer(void* arg) 
{
    int id = (intptr_t)arg;
    int cnt = 0;
    int pop_count = 0;
    int count = 0;
    Node* n;
    while (pop_count < OPS) 
    {
        n = dequeue(&q);
        if (n)
        {
            cnt = (int)(intptr_t)(n->value);
            free(n);
            pop_count++;
        }
        // usleep(1);      //-->Total ops: 2000000, Time: 28.50s, Rate: 70181 ops/sec 성능 안좋음!
        sched_yield();  //-->Total ops: 2000000, Time: 0.46s, Rate: 4376085 ops/sec
    }
    printf("th:%d pop: %d\n", id, cnt);
    return NULL;
}

int main() {
    initQueue(&q);

    //glibc pool에 미리 로딩ㅋㅋㅋㅋ
    const int N = 10000;
    Node *nodes[N];
    for (int i = 0; i < N; i++) 
    {
        nodes[i] = (Node *)malloc(sizeof(Node));
        if (!nodes[i]) {
            fprintf(stderr, "malloc failed at %d\n", i);
            exit(1);
        }
        nodes[i]->value = NULL;
        nodes[i]->next = 0;
    }

    printf("Allocated %d nodes\n", N);

    // 10,000개 해제
    for (int i = 0; i < N; i++) 
    {
        free(nodes[i]);
    }  


    pthread_t prod[THREADS], cons[THREADS];
    clock_gettime(CLOCK_MONOTONIC, &start);
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
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + 
                    (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Total ops: %d, Time: %.2fs, Rate: %.0f ops/sec\n", 
       THREADS * OPS, elapsed, (THREADS * OPS) / elapsed);
    printf("All threads finished.\n");
    if(dequeue(&q))printf("something wrong\n");
    // if(dequeue(&q))printf("something wrong\n");
    return 0;
}
