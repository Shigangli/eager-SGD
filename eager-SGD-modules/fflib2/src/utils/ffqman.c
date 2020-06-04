#include "ffqman.h"
#include "ffstorage.h"

#include <assert.h>

static pool_h qman_entry_pool;

void ffqman_entry_init(void * _entry){
    ffqman_entry_t * entry = (ffqman_entry_t *) _entry;
}

int ffqman_init(){
    qman_entry_pool = ffstorage_pool_create(sizeof(ffqman_entry_t), INITIAL_FFQMAN_ENTRY_COUNT, NULL);
    return FFSUCCESS;
}

int ffqman_finalize(){
    ffstorage_pool_destroy(qman_entry_pool);
    return FFSUCCESS;
}

int ffqman_create(ffqman_t * queue){
    FFLOCK_INIT(&(queue->lock));

    queue->head = NULL;
    queue->tail = NULL;

    queue->head;
    return FFSUCCESS;
}

int ffqman_free(ffqman_t * queue){
    return FFSUCCESS;
}

int ffqman_push(ffqman_t * queue, void * ptr){

    FFLOCK_LOCK(&(queue->lock));

    //FFLOG("PUSH %p head: %p; tail: %p\n", queue, queue->head, queue->tail);
    ffqman_entry_t * el;
    ffstorage_pool_get(qman_entry_pool, (void **) &el);

    el->next = NULL;
    el->ptr = ptr; 

    if (queue->tail!=NULL){
        queue->tail->next = el;
    } else {
        assert(queue->head==NULL);
        queue->head = el;
    }

    queue->tail = el;

    FFLOCK_UNLOCK(&(queue->lock));
    return FFSUCCESS;
}

int ffqman_pop(ffqman_t * queue, void ** ptr){
    FFLOCK_LOCK(&(queue->lock));

    //FFLOG("POP %p head: %p; tail: %p\n", queue, queue->head, queue->tail);


    ffqman_entry_t *el, *head, *head_next;

    //FFLOG("popping from %p\n", queue);

    if (queue->head == NULL){
        assert(queue->tail==NULL);
        *ptr = NULL;
    }else{
        ffqman_entry_t *head = queue->head;
        *ptr = head->ptr;
        
        queue->head = head->next;
        if (head->next == NULL){
            assert(queue->tail==head);
            queue->tail = NULL;
        }
        ffstorage_pool_put(head);
    }

    FFLOCK_UNLOCK(&(queue->lock));
    return FFSUCCESS;
}

