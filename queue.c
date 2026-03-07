#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>             //for uint64_t
#include <unistd.h>
#include "queue.h"

#define TAG_SHIFT 48
#define TAG_MASK  ((uint64_t)0xFFFF << TAG_SHIFT)
#define PTR_MASK  ((uint64_t)0x0000FFFFFFFFFFFF)


void initQueue(Queue* q)
{
    Node* dummy = malloc(sizeof(Node));
    dummy->value = (void*)(intptr_t)99999 ;
    dummy->next = 0;

    uint64_t tagptr = pack_tagged_ptr(dummy, 0);

    q->head = tagptr;
    q->tail = tagptr;
}

void enqueue(Queue* q,  void *value)
{
    Node *node = malloc(sizeof(Node));
    node->value = value;
    atomic_store_explicit(&node->next, (uint64_t)0, memory_order_relaxed); //태그 + 널 = 0 + 0 = 0

    uint64_t old_tail, new_next, tail_next, new_tail;
    while(1)
    {
        old_tail = atomic_load_explicit(&q->tail, memory_order_acquire);
        Node* old_ptr = unpack_ptr(old_tail);
        uint16_t old_tag = unpack_tag(old_tail);

        tail_next = atomic_load_explicit(&old_ptr->next, memory_order_acquire);
        Node* next = unpack_ptr(tail_next);
        uint16_t next_tag = unpack_tag(tail_next);

        //cas 전에 이미 변화 감지한 경우
        if(old_tail != atomic_load_explicit(&q->tail, memory_order_acquire))
        {
            sched_yield();
            continue;
        }

        if(next)
        {
            //tail인데 next가 잇는 경우 최신화
            //tail에서 aba 문제를 풀기 위해서 원래 tail의 태그에서 1 증가시켰음!
            new_tail = pack_tagged_ptr(next, old_tag + 1);
            atomic_compare_exchange_strong_explicit(&q->tail, &old_tail, new_tail,
                memory_order_acq_rel, memory_order_relaxed); //tail 최신화
            sched_yield();
        }
        else
        {
            //tail next의 값을 바꿀꺼니까 tail next의 tag 에서 1 증가
            new_next = pack_tagged_ptr(node, next_tag + 1);

            if(atomic_compare_exchange_strong_explicit(&old_ptr->next, &tail_next, new_next,
                memory_order_acq_rel, memory_order_relaxed))
            {
                //tail을 바꾸니까 테일 태그 기준으로
                new_tail = pack_tagged_ptr(node, old_tag + 1);
                atomic_compare_exchange_strong_explicit(&q->tail, &old_tail, new_tail,
                    memory_order_acq_rel, memory_order_relaxed);
                return;
            }
            sched_yield();
        }
    }
}

void enqueue_node(Queue* q, Node* node)
{
    atomic_store_explicit(&node->next, (uint64_t)0, memory_order_relaxed);

    uint64_t old_tail, new_next, tail_next, new_tail;
    while(1)
    {
        old_tail = atomic_load_explicit(&q->tail, memory_order_acquire);
        Node* old_ptr = unpack_ptr(old_tail);
        uint16_t old_tag = unpack_tag(old_tail);

        tail_next = atomic_load_explicit(&old_ptr->next, memory_order_acquire);
        Node* next = unpack_ptr(tail_next);
        uint16_t next_tag = unpack_tag(tail_next);

        if(old_tail != atomic_load_explicit(&q->tail, memory_order_acquire))
        {
            sched_yield();
            continue;
        }

        if(next)
        {
            new_tail = pack_tagged_ptr(next, old_tag + 1);
            atomic_compare_exchange_strong_explicit(&q->tail, &old_tail, new_tail,
                memory_order_acq_rel, memory_order_relaxed);
            sched_yield();
        }
        else
        {
            new_next = pack_tagged_ptr(node, next_tag + 1);
            if(atomic_compare_exchange_strong_explicit(&old_ptr->next, &tail_next, new_next,
                memory_order_acq_rel, memory_order_relaxed))
            {
                new_tail = pack_tagged_ptr(node, old_tag + 1);
                atomic_compare_exchange_strong_explicit(&q->tail, &old_tail, new_tail,
                    memory_order_acq_rel, memory_order_relaxed);
                return;
            }
            sched_yield();
        }
    }
}

Node* dequeue(Queue* q)
{
    uint64_t head, tail, next;
    Node* head_ptr, *next_ptr;
    void* pvalue;

    while(1)
    {
        head = atomic_load_explicit(&q->head, memory_order_acquire);
        tail = atomic_load_explicit(&q->tail, memory_order_acquire);
        head_ptr = unpack_ptr(head);
        next = atomic_load_explicit(&head_ptr->next, memory_order_acquire);
        next_ptr = unpack_ptr(next);

        if (head != atomic_load_explicit(&q->head, memory_order_acquire))
        {
            sched_yield();
            continue;
        }

        if (head_ptr == unpack_ptr(tail))
        {
            if (!next_ptr)
                return NULL;
            // tail이 뒤처짐 -> 전진시켜줌
            uint64_t new_tail = pack_tagged_ptr(next_ptr, unpack_tag(tail) + 1);
            atomic_compare_exchange_strong_explicit(&q->tail, &tail, new_tail,
                memory_order_acq_rel, memory_order_relaxed);
            sched_yield();
        }
        else
        {
            // CAS 전에 값을 먼저 읽어둠
            pvalue = next_ptr->value;
            uint64_t new_head = pack_tagged_ptr(next_ptr, unpack_tag(head) + 1);
            if (atomic_compare_exchange_strong_explicit(&q->head, &head, new_head,
                memory_order_acq_rel, memory_order_relaxed))
                break;
            sched_yield();
        }
    }

    // head_ptr = 옛 dummy (큐에서 빠짐, 해제 가능)
    // next_ptr = 새 dummy (큐에 남음)
    // pvalue = 꺼낸 값
    head_ptr->value = pvalue;
    return head_ptr;
}