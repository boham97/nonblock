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
    dummy->value = NULL;
    dummy->next = 0;

    uint64_t tagptr = pack_tagged_ptr(dummy, 0);

    q->head = tagptr;
    q->tail = tagptr;
}

void enqueue(Queue* q,  void *value) 
{
    Node *node = malloc(sizeof(Node));
    node->value = value;
    node->next = 0; //태그 + 널 = 0 + 0 = 0

    uint64_t old_tail, new_next, tail_next, new_tail;
    while(1)
    {
        old_tail = q->tail;
        Node* old_ptr = unpack_ptr(old_tail);
        uint16_t old_tag = unpack_tag(old_tail);
        
        tail_next = old_ptr->next;
        Node* next = unpack_ptr(tail_next);
        uint16_t next_tag = unpack_tag(tail_next);

        //cas 전에 이미 변화 감지한 경우
        if(old_tail != q->tail) 
        {
            continue;
        }

        if(next)
        {
            //tail인데 next가 잇는 경우 최신화
            //tail에서 aba 문제를 풀기 위해서 원래 tail의 태그에서 1 증가시켰음!
            new_tail = pack_tagged_ptr(next, old_tag + 1);  
            __sync_bool_compare_and_swap(&(q->tail), old_tail, new_tail); //tail 최신화
        }
        else
        {
            //tail next의 값을 바꿀꺼니까 tail next의 tag 에서 1 증가
            new_next = pack_tagged_ptr(node, next_tag + 1);

            if(__sync_bool_compare_and_swap(&old_ptr->next, tail_next, new_next))
            {
                //tail을 바꾸니까 테일 태그 기준으로
                new_tail = pack_tagged_ptr(node, old_tag + 1);
                __sync_bool_compare_and_swap(&q->tail, old_tail, new_tail);
                return;
            }
        }
    }
}

Node* dequeue(Queue* q) 
{
    uint64_t dummy_head, old_head, new_head;
    uint16_t old_tag;
    Node* node;
    Node* dummy;
    do
    {
        dummy_head = q->head;
        dummy = unpack_ptr(dummy_head);
 
        old_head = dummy->next;
        old_tag = unpack_tag(old_tag);
        node = unpack_ptr(old_head);
        if (!node) return NULL;



        new_head = pack_tagged_ptr(node, old_tag + 1);

    } while (!__sync_bool_compare_and_swap(&(dummy->next), old_head, new_head));

    return node;
}
