#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <signal.h>
#include "queue.h"

#define THREADS 12
#define OPS     1000000

Queue q;

/* ---------- 시그널 종료 플래그 ---------- */
volatile sig_atomic_t stop_flag = 0;

/* ---------- 스레드별 consume 카운터 ---------- */
long long consumed_per_thread[THREADS] = {0};

/* ---------- 배리어: 모든 스레드 동시 출발 ---------- */
pthread_barrier_t barrier;

/* ---------- SIGINT 핸들러 ---------- */
void handle_sigint(int sig)
{
    stop_flag = 1;
}

/* ---------- Producer ---------- */
void* producer(void* arg)
{
    int id = (intptr_t)arg;

    pthread_barrier_wait(&barrier);

    for (int i = 0; i < OPS; i++)
    {
        enqueue(&q, (void*)(intptr_t)(id * OPS + i + 1));
    }

    printf("th %d append: %d\n", id, OPS);
    return NULL;
}

/* ---------- Consumer ---------- */
void* consumer(void* arg)
{
    int id = (intptr_t)arg;
    int pop_count = 0;
    Node* n;

    pthread_barrier_wait(&barrier);

    while (!stop_flag && pop_count < OPS)
    {
        n = dequeue(&q);

        if (n)
        {
            //free(n);
            pop_count++;
            consumed_per_thread[id]++;
        }
        else
        {
            sched_yield();
        }
    }

    printf("th:%d pop: %lld\n", id, consumed_per_thread[id]);
    return NULL;
}

int main()
{
    signal(SIGINT, handle_sigint);

    /* 배리어 초기화: producer THREADS + consumer THREADS */
    pthread_barrier_init(&barrier, NULL, THREADS * 2);

    initQueue(&q);

    /* glibc malloc 워밍업 */
    const int N = 10000;
    Node *nodes[N];

    for (int i = 0; i < N; i++)
    {
        nodes[i] = (Node *)malloc(sizeof(Node));
        if (!nodes[i])
        {
            fprintf(stderr, "malloc failed at %d\n", i);
            exit(1);
        }
        nodes[i]->value = NULL;
        nodes[i]->next = 0;
    }

    for (int i = 0; i < N; i++)
        free(nodes[i]);

    pthread_t prod[THREADS], cons[THREADS];

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < THREADS; i++)
    {
        pthread_create(&prod[i], NULL, producer, (void*)(intptr_t)i);
        pthread_create(&cons[i], NULL, consumer, (void*)(intptr_t)i);
    }

    for (int i = 0; i < THREADS; i++)
        pthread_join(prod[i], NULL);

    for (int i = 0; i < THREADS; i++)
        pthread_join(cons[i], NULL);

    pthread_barrier_destroy(&barrier);

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed =
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("\n===== Consumed Per Thread =====\n");

    long long total = 0;
    for (int i = 0; i < THREADS; i++)
    {
        printf("Consumer %2d: %lld\n", i, consumed_per_thread[i]);
        total += consumed_per_thread[i];
    }

    printf("\nTotal consumed: %lld\n", total);
    printf("Elapsed Time : %.2f sec\n", elapsed);

    if (elapsed > 0)
        printf("Throughput : %.0f ops/sec\n", total / elapsed);

    printf("===============================\n");

    return 0;
}