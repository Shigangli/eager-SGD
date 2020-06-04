#ifndef _FFARMAN_H_
#define _FFARMAN_H_

#include "../ffinternal.h"
#include "../fflocks.h"

typedef struct entry {
    uint32_t idx;
    struct entry * next;
} entry_t;

typedef struct ffarman{
    /* entries in data that can be used */
    entry_t * free_entries;

    /* entries that can be reused as free_entries */    
    entry_t * free_free_entries;

    /* pointer to the memory to be freed */
    entry_t * tofree;
    
    uint32_t capacity;
    uint32_t count; //used only if FFDEBUG defined!

    FFLOCK_TYPE lock;

} ffarman_t;

int ffarman_create(uint32_t count, ffarman_t * array);
int ffarman_free(ffarman_t * array);
uint32_t ffarman_get(ffarman_t * array);
int ffarman_put(ffarman_t * array, uint32_t idx);

#endif /* _FFARMAN_H_ */
