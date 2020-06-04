#ifndef _FFSTORAGE_H_
#define _FFSTORAGE_H_

#include "ffinternal.h"

/* to avoid using another hashtable here*/
#define MAX_POOLS 128

typedef uint32_t pool_h;

typedef void (*ffpool_init_fun_t)(void *);

typedef struct ffmem_block{
    size_t size;
    uint32_t poolid;
    uint32_t id;

    /* used when the block is free */
    struct ffmem_block * next;
} ffmem_block_t;

typedef struct ffpool{
    ffmem_block_t * mem;
    ffmem_block_t * head;
    uint32_t pool_size;
    uint32_t curr_size;
    size_t elem_size;
    ffpool_init_fun_t init_fun;
} ffpool_t;


pool_h ffstorage_pool_create(size_t elem_size, uint32_t initial_count, ffpool_init_fun_t init_fun);
int ffstorage_pool_destroy(pool_h poolid);

int ffstorage_pool_get(pool_h pool, void ** ptr);
int ffstorage_pool_put(void * ptr);


int ffstorage_init();
int ffstorage_finalize();

#endif /* _FFSTORAGE_H */
