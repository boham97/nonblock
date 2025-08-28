#ifndef HASH_H
#define HASH_H

#define TRUE              1
#define FALSE             0
#define MAX_HASH_SIZE   256

typedef struct entry_st 
{
    unsigned long key;
    int value;
    struct entry_st *next;
    struct entry_st *next_trash;
} entry_st;


typedef struct hash_map
{
    entry_st *bucket[MAX_HASH_SIZE];
    int bucket_use[MAX_HASH_SIZE];
}hash_map;


unsigned int hash(unsigned long tid);
void hash_init(hash_map *map);
void hash_free(hash_map *map);
int hash_insert(hash_map * map, unsigned long tid, int value);
int hash_get(hash_map * map, unsigned long tid);
int hash_delete(hash_map * map);
int hash_delete_soft(hash_map * map, unsigned long tid);
void clean_trash(hash_map *map);



#endif