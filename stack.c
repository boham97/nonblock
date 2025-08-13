#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>             //for uint64_t
#include <unistd.h>

#define TAG_SHIFT 48
#define TAG_MASK  ((uint64_t)0xFFFF << TAG_SHIFT)
#define PTR_MASK  ((uint64_t)0x0000FFFFFFFFFFFF)



// Tagged pointer 구성
uint64_t pack_tagged_ptr(Node* ptr, uint16_t tag) 
{
    return ((uint64_t)tag << TAG_SHIFT) | ((uintptr_t)ptr & PTR_MASK);
}

// 포인터 추출
Node* unpack_ptr(uint64_t tagged)
{
    return (Node*)(tagged & PTR_MASK);
}

// 태그 추출
uint16_t unpack_tag(uint64_t tagged) 
{
    return (tagged >> TAG_SHIFT) & 0xFFFF;
}

void initStack(Stack* s) {
    s->top = 0;
}

void push(Stack* s, Node* node) 
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

Node* pop(Stack* s) 
{
    uint64_t old_top, new_top;
    Node* node;
    do
    {
        old_top = s->top;
        node = unpack_ptr(old_top);
        if (!node) return NULL;

        uint16_t old_tag = unpack_tag(old_top);
        Node* next = node->next;
        new_top = pack_tagged_ptr(next, old_tag + 1);

    } while (!__sync_bool_compare_and_swap(&(s->top), old_top, new_top));

    return node;
}
