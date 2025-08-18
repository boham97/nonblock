#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>  
#include "stack.h"
#include <unistd.h>

#define THREADS 10
#define OPS     100000

Stack stack;


// cc -g -o stack stack.c stack_test.c -lpthread



void* producer(void* arg) 
{
    int id = (intptr_t)arg;
    for (int i = 0; i < OPS; i++) 
    {
        Node* n = (Node*)malloc(sizeof(Node));
        n->value = (void*)(intptr_t)(id * OPS + i);
        push(&stack, n);
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
        n = pop(&stack);
        if (n)
        {
            cnt = (int)(intptr_t)n->value;
            free(n);
            pop_count++;
        }
    }
    return NULL;
}

int main() {
    initStack(&stack);

    pthread_t prod[THREADS], cons[THREADS];

    for (int i = 0; i < THREADS; i++) 
    {
        pthread_create(&prod[i], NULL, producer, (void*)(intptr_t)i);
        pthread_create(&cons[i], NULL, consumer, (void*)(intptr_t)i);
    }

    for (int i = 0; i < THREADS; i++) {
        pthread_join(prod[i], NULL);
    }
    for (int i = 0; i < THREADS; i++) {
        pthread_join(cons[i], NULL);
    }

    printf("All threads finished.\n");
    if(pop(&stack))printf("something wrong\n");
    return 0;
}
