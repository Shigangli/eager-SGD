
#include "ffarman.h"

#include <assert.h>

int ffarman_create(uint32_t count, ffarman_t * array){

    array->free_entries = (entry_t *) malloc(sizeof(entry_t) * (count+1));
    array->tofree = array->free_entries;
    array->capacity = count;
    array->count = 0;

    if (array->free_entries==NULL){
        return FFENOMEM;    
    }

    array->free_free_entries = NULL;

    for (uint32_t i=0; i<count; i++){
        array->free_entries[i].idx = i;
        array->free_entries[i].next = &(array->free_entries[i+1]);
    }
    array->free_entries[count-1].next = NULL;    

    FFLOCK_INIT(&(array->lock));

    return FFSUCCESS;
}

int ffarman_free(ffarman_t * array){
    free(array->tofree);
    FFLOCK_DESTROY(&(array->lock));
}

uint32_t ffarman_get(ffarman_t * array){   

    FFLOCK_LOCK(&(array->lock));
#ifdef FFDEBUG
    FFLOG("arman: %p  %u/%u\n", array, array->count, array->capacity);
#endif
    uint32_t res = -1;
    entry_t * e = array->free_entries;
    if (e != NULL) {


        /* remove from free_entries */
        array->free_entries = e->next;

        /* add to free_free entries */
        e->next = array->free_free_entries;
        array->free_free_entries = e;
        res = e->idx;
    }   
 #ifdef FFDEBUG
    array->count++;
#endif
    FFLOCK_UNLOCK(&(array->lock));
    /* return the index */
    //assert(res!=-1);
    return res;
}

int ffarman_put(ffarman_t * array, uint32_t idx){

    FFLOCK_LOCK(&(array->lock));
    /* take an entry that we can use to store the free_entry */

#ifdef FFDEBUG
    FFLOG("arman: %p  %u/%u\n", array, array->count, array->capacity);
#endif
    entry_t * e = array->free_free_entries;
    assert(e!=NULL);

    array->free_free_entries = e->next;    

    e->idx = idx;
    
    /* make the entry available */
    e->next = array->free_entries;
    array->free_entries = e;
#ifdef FFDEBUG
    array->count--;
#endif
    FFLOCK_UNLOCK(&(array->lock));    

    return FFSUCCESS;
}


