#include <stdlib.h>
#include "hash.h"


void hash_init(hash_map *map)
{
    int i = 0;
    for (i = 0; i < MAX_HASH_SIZE; i ++)
    {
        map->bucket[i] = NULL;
        map->bucket_use[i] = FALSE;
    }
}

unsigned int hash(unsigned long tid)
{
    //return ((uintptr_t) tid) % MAX_HASH_SIZE;
    return tid % MAX_HASH_SIZE;
}

//마지막 엔트리 CAS 획득 필요
//d_flag 이면 넣기
int hash_insert(hash_map *map, unsigned long tid, int value)
{
    int index = hash(tid);

    entry_st *new_entry = NULL;
    new_entry = (entry_st *)malloc(sizeof(entry_st));
    if (! new_entry)return FALSE;

    new_entry->key = tid;
    new_entry->value = value;
    new_entry->next = NULL;
    
    printf("hash_insert: %d %lu\n", index, tid);
    get_lock(map, index);
    entry_st *entry = map->bucket[index];
    if (entry == NULL)
    {
        map->bucket[index] = new_entry;
        release_lock(map, index);
        printf("hash_insert 1\n");
        return TRUE;
    }

    while(entry)
    {
        if(entry->key == new_entry->key)
        {
            entry->value = value;
            free(new_entry);
            release_lock(map, index);
            printf("hash_insert 2\n");
            return TRUE;
        }
        else if (entry->next == NULL)
        {
            entry->next = new_entry;    
            release_lock(map, index);
            printf("hash_insert 3\n");
            return TRUE;
        }
    }
    release_lock(map, index);

    return FALSE;
}

int hash_get(hash_map * map, unsigned long tid)
{
    int index = hash(tid);
    entry_st *entry = map->bucket[index];
    while (entry) 
    {
        if (tid == entry->key)
        {
            return entry->value;
        }
        entry = entry->next;
    }
    return FALSE;  // 키를 찾지 못한 경우

}


int hash_delete(hash_map *map, unsigned long tid)
{
    int index = hash(tid);
    entry_st *curr = map->bucket[index];
    entry_st *prev = NULL;
    get_lock(map, index);
    while (curr) 
    {
        if (tid == curr->key)
        {
            if(prev)
                __sync_bool_compare_and_swap(&prev->next, curr, curr->next);
            else
                __sync_bool_compare_and_swap(&map->bucket[index], curr, curr->next);

            free(curr);                                     //위험 수정 필요!: FREE할떄 읽고 있을수 있음
            release_lock(map, index);
            return TRUE;
        }
        prev = curr;
        curr = curr->next;
    }
    release_lock(map, index);
    return FALSE;  // 키를 찾지 못한 경우
}


int get_lock(hash_map *map, int index)
{
    while (!__sync_bool_compare_and_swap(&map->bucket_use[index], FALSE, TRUE));
    return TRUE;
}

//한 쓰레드만 이미 접근했으므로 원자적 연산X
int release_lock(hash_map *map, int index)
{
    map->bucket_use[index] = FALSE;
    return TRUE;
}