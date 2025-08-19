#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>             //for uint64_t
#include <unistd.h>
#include "queue.h"

#define TAG_SHIFT 48
#define TAG_MASK  ((uint64_t)0xFFFF << TAG_SHIFT)
#define PTR_MASK  ((uint64_t)0x0000FFFFFFFFFFFF)


Queue *queue_init()
{
    Queue* q = malloc(sizeof(Queue));
    Node* dummy = malloc(sizeof(Node));
    dummy->value = NULL;
    dummy->next = NULL;

    uint64_t tagptr = pack_tagged_ptr(dummy, 0);

    q->head = tagptr;
    q->tail = tagptr;

    return q;
}

void push(Queue* q, Node* node) 
{
    uint64_t old_top, new_top;
    do 
    {
        old_top = s->top;
        Node* old_ptr = unpack_ptr(old_top);
        uint16_t old_tag = unpack_tag(old_top);
        node->next = old_ptr;
        new_top = pack_tagged_ptr(node, old_tag + 1);
    } while (!__sync_bool_compare_and_swap(&(s->top), old_top, new_top));
}

Node* pop(Queue* q) 
{
    uint64_t old_head, new_head;
    Node* node;
    do
    {
        old_head = q->head;
        node = unpack_ptr(old_head);
        if (!node) return NULL;

        uint16_t old_tag = unpack_tag(old_head);
        Node* next = node->next;
        new_head = pack_tagged_ptr(next, old_tag + 1);

    } while (!__sync_bool_compare_and_swap(&(q->head), old_head, new_head));

    return node;
}
