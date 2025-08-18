#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>


typedef struct Node {
    void *value;
    struct Node* next;
} Node;

typedef struct Queue {
    uint64_t head; // Tagged pointer: [태그(16bit) | 포인터(48bit)]
    uint64_t tail;
} Queue;

// Tagged pointer 관련 매크로
#define TAG_SHIFT 48
#define TAG_MASK  ((uint64_t)0xFFFF << TAG_SHIFT)
#define PTR_MASK  ((uint64_t)0x0000FFFFFFFFFFFF)


// Tagged pointer 구성
static inline uint64_t pack_tagged_ptr(Node* ptr, uint16_t tag) 
{
    return ((uint64_t)tag << TAG_SHIFT) | ((uintptr_t)ptr & PTR_MASK);
}

// 포인터 추출
static inline Node* unpack_ptr(uint64_t tagged)
{
    return (Node*)(tagged & PTR_MASK);
}

// 태그 추출
static inline uint16_t unpack_tag(uint64_t tagged) 
{
    return (tagged >> TAG_SHIFT) & 0xFFFF;
}

static inline void initQueue(Queue* q) 
{
    q->top = 0;
}

// push/pop 함수 선언
void push(Queue* q, Node* node);
Node* pop(Queue* q);

#endif // QUEUE_H
