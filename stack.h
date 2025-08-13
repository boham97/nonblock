#ifndef STACK_H
#define STACK_H

#include <stdint.h>


typedef struct Node {
    int value;
    struct Node* next;
} Node;

typedef struct Stack {
    uint64_t top; // Tagged pointer: [태그(16bit) | 포인터(48bit)]
} Stack;

// Tagged pointer 관련 매크로
#define TAG_SHIFT 48
#define TAG_MASK  ((uint64_t)0xFFFF << TAG_SHIFT)
#define PTR_MASK  ((uint64_t)0x0000FFFFFFFFFFFF)

static inline Node* get_ptr(uint64_t tagged) {
    return (Node*)(tagged & PTR_MASK);
}

static inline uint16_t get_tag(uint64_t tagged) {
    return (uint16_t)((tagged >> 48) & 0xFFFF);
}

static inline uint64_t make_tagged(Node* ptr, uint16_t tag) {
    return ((uint64_t)tag << 48) | ((uint64_t)ptr & PTR_MASK);
}

// 스택 초기화
static inline void initStack(Stack* s) {
    s->top = 0; // NULL 포인터 + tag=0
}

// 스택이 비었는지 확인
static inline bool isEmpty(Stack* s) {
    return s->top == 0;
}

// push/pop 함수 선언
bool push(Stack* s, int value);
bool pop(Stack* s, int* value);

#endif // STACK_H
