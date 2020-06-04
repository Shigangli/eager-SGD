#ifndef _FFQMAN_H_
#define _FFQMAN_H_

#include "ffinternal.h"
#include "fflocks.h"

#define INITIAL_FFQMAN_ENTRY_COUNT 2048

typedef struct ffqman_entry {
    void * ptr;
    struct ffqman_entry * next;
    struct ffqman_entry * prev;
} ffqman_entry_t;

typedef struct ffqman {
    ffqman_entry_t *head, *tail;
    ffqman_entry_t *head_sentinel, *tail_sentinel;
    FFLOCK_TYPE lock;

} ffqman_t;


int ffqman_init();
int ffqman_finalize();
int ffqman_create(ffqman_t * queue);
int ffqman_free(ffqman_t * queue);
int ffqman_push(ffqman_t * queue, void * ptr);
int ffqman_pop(ffqman_t * queue, void ** ptr);

#endif /* _FFQMAN_H_ */
