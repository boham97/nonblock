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

/* ---------- Producer 인자: id + 미리 할당된 노드 풀 ---------- */
typedef struct {
    int   id;
    Node *pool;
} ProdArg;

/* ---------- SIGINT 핸들러 ---------- */
void handle_sigint(int sig)
{
    stop_flag = 1;
}

/* ---------- Producer ---------- */
void* producer(void* arg)
{
    ProdArg *pa = (ProdArg *)arg;
    int id      = pa->id;
    Node *pool  = pa->pool;

    pthread_barrier_wait(&barrier);

    for (int i = 0; i < OPS; i++)
    {
        pool[i].value = (void*)(intptr_t)(id * OPS + i + 1);
        enqueue_node(&q, &pool[i]);
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

    /* 배리어 초기화: producer THREADS + consumer THREADS + main */
    pthread_barrier_init(&barrier, NULL, THREADS * 2 + 1);

    initQueue(&q);

    /* 스레드별 노드 풀 미리 할당 (측정 전) */
    Node    *pools[THREADS];
    ProdArg  prod_args[THREADS];

    for (int i = 0; i < THREADS; i++)
    {
        pools[i] = (Node *)malloc(OPS * sizeof(Node));
        if (!pools[i])
        {
            fprintf(stderr, "pool malloc failed at thread %d\n", i);
            exit(1);
        }
        prod_args[i].id   = i;
        prod_args[i].pool = pools[i];
    }

    pthread_t prod[THREADS], cons[THREADS];

    struct timespec start, end;

    for (int i = 0; i < THREADS; i++)
    {
        pthread_create(&prod[i], NULL, producer, &prod_args[i]);
        pthread_create(&cons[i], NULL, consumer, (void*)(intptr_t)i);
    }

    /* 모든 스레드가 배리어에 도달하면 start를 찍고 동시에 풀어줌 */
    clock_gettime(CLOCK_MONOTONIC, &start);
    pthread_barrier_wait(&barrier);

    for (int i = 0; i < THREADS; i++)
        pthread_join(prod[i], NULL);

    for (int i = 0; i < THREADS; i++)
        pthread_join(cons[i], NULL);

    pthread_barrier_destroy(&barrier);

    clock_gettime(CLOCK_MONOTONIC, &end);

    /* 풀 해제 */
    for (int i = 0; i < THREADS; i++)
        free(pools[i]);

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
